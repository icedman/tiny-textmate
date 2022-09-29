#include "textmate.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

TxSyntaxNode *tx_syntax_from_path(char_u *path) {
  char_u file_name[MAX_PATH_LENGTH] = "";

  // extract filename
  char_u *last_separator = strrchr(path, DIR_SEPARATOR);
  if (last_separator) {
    strcpy(file_name, last_separator + 1);
  } else {
    strcpy(file_name, path);
  }

  // extract extension
  char_u *extension = strrchr(file_name, '.');
  // printf("%s %s\n", file_name, extension);

  TxNode *packages = tx_global_packages();
  TxNode *grammars = NULL;
  TxNode *language = NULL;

  // query languages
  TxNode *child = packages->first_child;
  while (child) {
    TxPackage *pk = txn_package_value(child);
    grammars = pk->grammars;
    TxNode *languages = pk->languages;
    if (languages) {
      TxNode *lang_child = languages->first_child;
      while (lang_child) {
        TxNode *lang_filenames = txn_get(lang_child, "filenames");
        if (lang_filenames) {
          TxNode *fname_child = lang_filenames->first_child;
          while (fname_child) {
            if (strcmp(fname_child->string_value, file_name) == 0) {
              language = lang_child;
              goto lang_found;
            }
            fname_child = fname_child->next_sibling;
          }
        }
        TxNode *lang_extensions = txn_get(lang_child, "extensions");
        if (lang_extensions && extension) {
          TxNode *ext_child = lang_extensions->first_child;
          while (ext_child) {
            if (strcmp(ext_child->string_value, extension) == 0) {
              language = lang_child;
              goto lang_found;
            }
            ext_child = ext_child->next_sibling;
          }
        }
        lang_child = lang_child->next_sibling;
      }
    }
    child = child->next_sibling;
  }

lang_found:

  if (language) {
    TxNode *lang_id = txn_get(language, "id");
    char temp[TX_MAX_LINE_LENGTH];
    sprintf(temp, "source.%s", lang_id->string_value);
    TxSyntaxNode *res = tx_syntax_from_scope(temp);

    if (!res && grammars) {
      TxNode *grammar = grammars->first_child;
      while (grammar) {
        TxNode *grammar_language = txn_get(grammar, "language");
        if (strcmp(grammar_language->string_value, lang_id->string_value) ==
            0) {
          TxNode *scope = txn_get(grammar, "scopeName");
          if (scope) {
            return tx_syntax_from_scope(scope->string_value);
          }
        }
        grammar = grammar->next_sibling;
      }
    }
    return res;
  }

  // TX_LOG("not found!\n");
  return NULL;
}

TxSyntaxNode *tx_syntax_from_scope(char_u *scope) {
  // check global repository
  TxSyntaxNode *syntax_node = txn_get(tx_global_repository(), scope);
  if (syntax_node) {
    // TX_LOG("found at repository!\n");
    return syntax_node;
  }

  // check packages
  TxNode *packages = tx_global_packages();
  TxNode *child = packages->first_child;
  while (child) {
    TxPackage *pk = txn_package_value(child);
    TxNode *grammars = pk->grammars;
    if (grammars) {
      TxNode *grammar_node = txn_get(grammars, scope);
      if (grammar_node) {
        TxNode *path = txn_get(grammar_node, "fullPath");
        if (path) {
          // printf("%s\n", path->string_value);
          TxSyntaxNode *syntax_node = txn_load_syntax(path->string_value);
          if (syntax_node) {
            return txn_set(tx_global_repository(), scope, syntax_node);
          } else {
            // dummy syntax
            return txn_set(tx_global_repository(), scope, txn_new_syntax());
          }
        }
      }
    }
    child = child->next_sibling;
  }
  return NULL;
}

void tx_read_package_dir(char *path) {
  DIR *dp;
  struct dirent *ep;

  char_u base_path[MAX_PATH_LENGTH];
  sprintf(base_path, "%s/", path);
  dp = opendir(base_path);
  // TX_LOG("%s\n", base_path);
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

TxThemeNode *tx_theme_from_name(char_u *name) {
  TxThemeNode *res = NULL;

  // check packages
  TxNode *packages = tx_global_packages();
  TxNode *child = packages->first_child;
  while (child) {
    TxPackage *pk = txn_package_value(child);
    TxNode *themes = pk->themes;
    if (themes) {
    }
    child = child->next_sibling;
  }

  return res;
}