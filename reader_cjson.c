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
        // self or base
      } else {
        // external?
        syntax->include_external = true;
        printf("external %s\n", path);
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
 * 2. pre-compile regex
 * 3. set pointers to nodes in TxSyntax
 */
static void parse_syntax(cJSON *obj, TxSyntaxNode *root, TxSyntaxNode *node) {
  TxSyntax *syntax = txn_syntax_value(node);

  // regex
  {
    char_u *keys[] = {// "firstLineMatch",
                      // "foldingStartMarker",
                      // "foldingStopMarker",
                      "match", "begin", "end", 0};

    regex_t **regexes[] = {// &syntax->rx_first_line_match,
                           // &syntax->rx_folding_start_marker,
                           // &syntax->rx_folding_Stop_marker,
                           &syntax->rx_match, &syntax->rx_begin,
                           &syntax->rx_end};

    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item && item->valuestring) {
        TxSyntaxNode *regex_node = txn_new_syntax();
        if (item->valuestring) {
          if (txn_syntax_value(root)->verbose) {
            txn_set_string_value(regex_node, item->valuestring);
          }
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
        if (i == 2) {
          syntax->name = value->string_value;
        }
        if (i == 3) {
          syntax->content_name = value->string_value;
        }
        if (i == 4) {
          syntax->scope_name = value->string_value;
        }
      }
    }
  }

  // captures
  {
    char_u *keys[] = {"captures", "beginCaptures", "endCaptures", 0};

    for (int i = 0;; i++) {
      char_u *key = keys[i];
      if (!key)
        break;

      cJSON *item = cJSON_GetObjectItem(obj, key);
      if (item) {
        TxSyntaxNode *captures = txn_new_syntax();
        txn_set(node, key, captures);
        if (i == 2) {
          syntax->end_captures = captures;
        } else {
          syntax->captures = captures;
        }
        char_u *capture_keys[] = {"0",  "1",  "2",  "3",  "4",  "5",
                                  "6",  "7",  "8",  "9",  "10", "11",
                                  "12", "13", "14", "15", 0};
        for (int j = 0;; j++) {
          char_u *capture_key = capture_keys[j];
          if (!capture_key)
            break;
          cJSON *capture_item = cJSON_GetObjectItem(item, capture_key);
          if (capture_item) {
            TxSyntaxNode *capture_node = txn_new_syntax();
            capture_node->self.number_value = i;
            txn_set(captures, capture_key, capture_node);
            parse_syntax(capture_item, root, capture_node);
          }
        }
      }
    }
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

// re-implement these next functions if you want to ditch cjson

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
  syntax->verbose = true;
  syntax->repository = txn_new_object();

  txn_set(root, "repository", syntax->repository);
  parse_syntax(json, root, root);

  cJSON_free(content);

  post_process_syntax(root);
  return root;
}

TxThemeNode *txn_load_theme(char_u *path) { return NULL; }

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

  cJSON_free(content);
  return pkn;
}

void tx_read_package_dir(char *path) {
  char_u *base_path = path;
  DIR *dp;
  struct dirent *ep;
  dp = opendir(path);
  if (dp != NULL) {
    while ((ep = readdir(dp)) != NULL) {
      char_u full_path[MAX_PATH_LENGTH] = "";
      char_u package_path[MAX_PATH_LENGTH] = "";
      char_u *last_separator = strrchr(base_path, DIR_SEPARATOR);
      char_u *relative_path = ep->d_name;

      strcpy(full_path, base_path);
      if (!last_separator) {
        sprintf(full_path + strlen(base_path), "%s", relative_path);
      } else {
        sprintf(full_path + (last_separator - base_path) + 1, "%s",
                relative_path);
      }
      sprintf(package_path, "%s%cpackage.json", full_path, DIR_SEPARATOR);

      // printf("%s\n", package_path);

      TxPackageNode *pkn = txn_load_package(package_path);
      if (pkn) {
        // dump(pkn, 0);
        txn_push(tx_global_packages(), pkn);
      }
    }
    closedir(dp);
  }
}