#include "cJSON.h"
#include "textmate.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void post_process_syntax(TxSyntaxNode *n) {
  TxSyntax *syntax = txn_syntax_value(n);
  TxNode *root = txn_root(n);
  if (syntax) {
    TxNode *repo_node = txn_syntax_value(root)->repository;
    TxNode *include_node = txn_get(n, "include");
    if (include_node && include_node->string_value && !syntax->include) {
      char_u *path = include_node->string_value;

      if (path[0] == '#') {
        path++;
      } else if (path[0] == '$') {
        if (strcmp(path, "$self") == 0) {
          syntax->include = root;
        } else if (strcmp(path, "$base") == 0) {
          // todo ... for embed grammars
          syntax->include = root;
        }
      } else {
        // external?
        syntax->include_external = true;
        // printf("external %s\n", path);
      }

      if (repo_node) {
        TxNode *include_node = txn_get(repo_node, path);
        if (include_node) {
          syntax->include = include_node;
          // printf("include! %s\n", path);
        }
      }
    }
  }

  TxSyntaxNode *c = n->self.first_child;
  while (c) {
    post_process_syntax(c);
    c = c->self.next_sibling;
  }
}

/*
 * parsed syntax must:
 * 1. resolve local includes
 * 2. pre-compile regex, and pre-determine for dynamic end matches
 * 3. set pointers to nodes in TxSyntax
 */
static void parse_syntax(cJSON *obj, TxSyntaxNode *root, TxSyntaxNode *node) {
  TxSyntax *syntax = txn_syntax_value(node);

  // regex
  {
    char_u *keys[] = {"match", "begin", "end", 0};

    regex_t **regexes[] = {&syntax->rx_match, &syntax->rx_begin,
                           &syntax->rx_end};

    char_u **regexes_strings[] = {&syntax->rxs_match, &syntax->rxs_begin,
                                  &syntax->rxs_end};

    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item && item->valuestring) {
        TxSyntaxNode *regex_node = txn_new_syntax();
        if (item->valuestring) {

#ifdef TX_SYNTAX_VERBOSE_REGEX
          txn_set_string_value(regex_node, item->valuestring);
          *regexes_strings[i] = regex_node->self.string_value;
#endif

#ifdef TX_SYNTAX_RECOMPILE_REGEX_END
          if (strcmp(key, "end") == 0) {
            char_u *capture_keys[] = {"\\1",  "\\2",  "\\3", "\\4",  "\\5",
                                      "\\6",  "\\7",  "\\8", "\\9",  "\\10",
                                      "\\11", "\\12", "\13", "\\14", "\\15",
                                      "\\16", 0};
            for (int j = 0; j < TX_MAX_MATCHES; j++) {
              if (strstr(item->valuestring, capture_keys[j])) {
                TxNode *ns = txn_new_string(item->valuestring);
                txn_set(node, "end_pattern", ns);
                txn_syntax_value(node)->end_pattern = ns->string_value;
                break;
              }
            }
          }
#endif

          *regexes[i] = tx_compile_pattern(item->valuestring);
        }
        txn_set(node, key, regex_node);
      }
    }
  }

  // strings
  {
    char_u *keys[] = {"content",   "fileTypes",     "name",    "contentName",
                      "scopeName", "keyEquivalent", "comment", 0};

    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item && item->valuestring) {
        TxNode *value = txn_set(node, key, txn_new_string(item->valuestring));
        if (strcmp(key, "name") == 0) {
          syntax->name = value->string_value;
        }
        if (strcmp(key, "contentName") == 0) {
          syntax->content_name = value->string_value;
        }
        if (strcmp(key, "scopeName") == 0) {
          syntax->scope_name = value->string_value;
        }
      }
    }
  }

  // captures
  {
    char_u *keys[] = {"captures", "beginCaptures", "endCaptures", 0};
    TxSyntaxNode **capture_nodes[] = {
        &syntax->captures, &syntax->begin_captures, &syntax->end_captures};
    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item) {
        TxSyntaxNode *captures = txn_new_syntax();
        txn_set(node, key, captures);
        *capture_nodes[i] = captures;

        char_u *capture_keys[] = {"1", "2", "3", "4", "5",
                                  "6", "7", "8", "9", 0};
        for (int j = 0; j < TX_MAX_MATCHES; j++) {
          char_u *capture_key = capture_keys[j];
          if (!capture_key)
            break;
          cJSON *capture_item = cJSON_GetObjectItem(item, capture_key);
          if (capture_item) {
            TxSyntaxNode *capture_node = txn_new_syntax();
            txn_set(captures, capture_key, capture_node);
            parse_syntax(capture_item, root, capture_node);
          }
        }
      }
    }

    // if (!syntax->end_captures) {
    //   syntax->end_captures = syntax->captures;
    // }
  }

  // patterns
  {
    TxSyntaxNode *patterns = NULL;
    cJSON *item = cJSON_GetObjectItem(obj, "patterns");
    cJSON *child = item ? item->child : NULL;
    if (child) {
      patterns = txn_new_array();
      syntax->patterns = patterns;
      txn_set(node, "patterns", patterns);
    }
    while (child) {
      TxSyntaxNode *pattern = txn_new_syntax();
      txn_push(patterns, pattern);
      parse_syntax(child, node, pattern);
      child = child->next;
    }
  }

  // include
  {
    cJSON *item = cJSON_GetObjectItem(obj, "include");
    if (item && item->valuestring) {
      txn_set(node, "include", txn_new_string(item->valuestring));
    }
  }

  // repository
  {
    TxNode *root_repo = txn_syntax_value(root)->repository;
    if (root_repo) {
      cJSON *item = cJSON_GetObjectItem(obj, "repository");
      if (item) {
        size_t sz = cJSON_GetArraySize(item);
        for (int i = 0; i < sz; i++) {
          cJSON *repo_item = cJSON_GetArrayItem(item, i);
          TxSyntaxNode *repo_node = txn_new_syntax();
          txn_set(root_repo, repo_item->string, repo_node);
          parse_syntax(repo_item, root, repo_node);
        }
      }
    }
  }
}

/*
 * parsed package must:
 * 1. set pointers to nodes in TxPackage (grammars, languages, themes)
 * 2. languages must resolve scope_name
 */
static void parse_package(cJSON *obj, TxPackageNode *node, char_u *path) {
  char_u *base_path = path;
  TxPackage *package = txn_package_value(node);
  TxNode *grammars_node = txn_get(node, "grammars");
  if (!grammars_node) {
    grammars_node = txn_set(node, "grammars", txn_new_object());
  }
  package->grammars = grammars_node;
  TxNode *languages_node = txn_get(node, "languages");
  if (!languages_node) {
    languages_node = txn_set(node, "languages", txn_new_object());
  }
  package->languages = languages_node;

  // keys
  {
    char_u *keys[] = {"name",    "displayName", "description",
                      "version", "publisher",   0};
    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item && item->valuestring) {
        txn_set(node, key, txn_new_string(item->valuestring));
      }
    }
  }

  {
    cJSON *contributes = cJSON_GetObjectItem(obj, "contributes");
    {
      cJSON *grammars = cJSON_GetObjectItem(contributes, "grammars");
      if (grammars) {
        size_t sz = cJSON_GetArraySize(grammars);
        for (int i = 0; i < sz; i++) {
          cJSON *grammar_item = cJSON_GetArrayItem(grammars, i);
          TxNode *grammar_node = txn_new_object();
          char_u *keys[] = {"language", "path", "scopeName", 0};
          char_u *scope_name = NULL;
          char_u *relative_path = NULL;

          for (int j = 0;; j++) {
            char_u *key = keys[j];
            if (!key)
              break;

            cJSON *item = cJSON_GetObjectItem(grammar_item, key);
            if (item && item->valuestring) {
              txn_set(grammar_node, key, txn_new_string(item->valuestring));
              if (j == 1) {
                relative_path = item->valuestring;
              }
              if (j == 2) { // scope_name
                scope_name = item->valuestring;
              }
            }
          }

          if (scope_name && relative_path && base_path) {
            txn_set(grammars_node, scope_name, grammar_node);
            char_u full_path[MAX_PATH_LENGTH] = "";
            char_u *last_separator = strrchr(base_path, DIR_SEPARATOR);
            strcpy(full_path, base_path);
            if (!last_separator) {
              sprintf(full_path + strlen(base_path), "%s", relative_path);
            } else {
              sprintf(full_path + (last_separator - base_path) + 1, "%s",
                      relative_path);
            }
            txn_set(grammar_node, "fullPath", txn_new_string(full_path));
          } else {
            txn_free(grammar_node);
          }
        }
      }

      cJSON *languages = cJSON_GetObjectItem(contributes, "languages");
      if (languages) {
        size_t sz = cJSON_GetArraySize(languages);
        for (int i = 0; i < sz; i++) {
          cJSON *language_item = cJSON_GetArrayItem(languages, i);
          TxNode *language_node = txn_new_object();
          char_u *keys[] = {"id", 0};
          char_u *language_id = NULL;
          for (int j = 0;; j++) {
            char_u *key = keys[j];
            if (!key)
              break;
            cJSON *item = cJSON_GetObjectItem(language_item, key);
            if (item && item->valuestring) {
              txn_set(language_node, key, txn_new_string(item->valuestring));
              if (j == 0) {
                language_id = item->valuestring;
              }
            }
          }
          if (language_id) {
            txn_set(languages_node, language_id, language_node);
            TxNode *extensions_node =
                txn_set(language_node, "extensions", txn_new_array());
            TxNode *filenames_node =
                txn_set(language_node, "filenames", txn_new_array());

            // resolve grammar scope_name
            TxNode *grammar_child = grammars_node->first_child;
            while (grammar_child) {
              TxNode *id = txn_get(grammar_child, "language");
              TxNode *scope = txn_get(grammar_child, "scopeName");
              if (scope && id && strcmp(id->string_value, language_id) == 0) {
                txn_set(language_node, "scopeName",
                        txn_new_string(scope->string_value));
                break;
              }
              grammar_child = grammar_child->next_sibling;
            }

            // extensions
            cJSON *extensions =
                cJSON_GetObjectItem(language_item, "extensions");
            if (extensions) {
              size_t ext_sz = cJSON_GetArraySize(extensions);
              for (int j = 0; j < ext_sz; j++) {
                cJSON *extension_item = cJSON_GetArrayItem(extensions, j);
                if (extension_item->valuestring) {
                  txn_push(extensions_node,
                           txn_new_string(extension_item->valuestring));
                }
              }
            }

            // filenames
            cJSON *filenames = cJSON_GetObjectItem(language_item, "filenames");
            if (filenames) {
              size_t ext_sz = cJSON_GetArraySize(filenames);
              for (int j = 0; j < ext_sz; j++) {
                cJSON *filename_item = cJSON_GetArrayItem(filenames, j);
                if (filename_item->valuestring) {
                  txn_push(filenames_node,
                           txn_new_string(filename_item->valuestring));
                }
              }
            }

          } else {
            txn_free(language_node);
          }
        }
      }

      // cJSON *themes = cJSON_GetObjectItem(contributes, "themes");
    }
  }
}

static void parse_theme(cJSON *obj, TxThemeNode *node, char_u *path) {
  char_u *base_path = path;
  TxTheme *theme = txn_theme_value(node);
  TxNode *token_colors = txn_get(node, "tokenColors");
  if (!token_colors) {
    token_colors = txn_set(node, "tokenColors", txn_new_object());
    theme->token_colors = token_colors;
  }

  {
    cJSON *colors = cJSON_GetObjectItem(obj, "colors");
    if (colors) {
      cJSON *item = colors->child;
      while (item) {
        // printf("%s:%s\n", item->string, item->valuestring);
        if (item->valuestring) {
          TxNode *n = txn_set(token_colors, item->string,
                              txn_new_string(item->valuestring));
          txt_parse_color(item->valuestring, &n->number_value);
        }
        item = item->next;
      }
    }
  }

  {
    cJSON *tokenColors = cJSON_GetObjectItem(obj, "tokenColors");
    if (tokenColors) {
      size_t sz = cJSON_GetArraySize(tokenColors);
      for (int i = 0; i < sz; i++) {
        cJSON *token_item = cJSON_GetArrayItem(tokenColors, i);
        cJSON *scope = cJSON_GetObjectItem(token_item, "scope");
        cJSON *settings = cJSON_GetObjectItem(token_item, "settings");
        cJSON *fg = NULL;

        if (!settings)
          continue;
        fg = cJSON_GetObjectItem(settings, "foreground");
        if (!fg)
          continue;

        if (scope && scope->valuestring) {
          TxNode *n = txn_set(token_colors, scope->valuestring,
                              txn_new_string(fg->valuestring));
          txt_parse_color(fg->valuestring, &n->number_value);
        } else if (scope) {
          size_t scopes = cJSON_GetArraySize(scope);
          for (int j = 0; j < scopes; j++) {
            cJSON *scope_item = cJSON_GetArrayItem(scope, j);
            if (scope_item && scope_item->valuestring) {
              TxNode *n = txn_set(token_colors, scope_item->valuestring,
                                  txn_new_string(fg->valuestring));
              txt_parse_color(fg->valuestring, &n->number_value);
            }
          }
        }
      }
    }
  }
}

// --------------------
// re-implement these next functions if you want to ditch cjson
// --------------------
TxSyntaxNode *txn_load_syntax(char_u *path) {
  FILE *fp = fopen(path, "r");

  if (!fp) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  size_t sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char_u *content = tx_malloc(sz);
  fread(content, sz, 1, fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(content);

  TxSyntaxNode *root = txn_new_syntax();
  TxSyntax *syntax = txn_syntax_value(root);
  txn_set_name(root, "root");
  syntax->repository = txn_new_object();

  txn_set(root, "repository", syntax->repository);
  parse_syntax(json, root, root);

  cJSON_free(json);
  tx_free(content);

  post_process_syntax(root);
  return root;
}

TxThemeNode *txn_load_theme(char_u *path) {
  FILE *fp = fopen(path, "r");

  if (!fp) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  size_t sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char_u *content = tx_malloc(sz);
  fread(content, sz, 1, fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(content);

  TxThemeNode *thm = txn_new_theme();
  parse_theme(json, thm, path);

  cJSON_free(json);
  tx_free(content);
  return thm;
}

TxPackageNode *txn_load_package(char_u *path) {
  FILE *fp = fopen(path, "r");

  if (!fp) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  size_t sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char_u *content = tx_malloc(sz);
  fread(content, sz, 1, fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(content);

  TxPackageNode *pkn = txn_new_package();
  parse_package(json, pkn, path);

  cJSON_free(json);
  tx_free(content);
  return pkn;
}
