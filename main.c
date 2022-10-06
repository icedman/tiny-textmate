#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static char _debug = 'r';

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  tx_initialize();

  char *default_path = "./samples/simple.c";
  char *path = default_path;

  char *default_grammar_path = NULL;
  char *grammar_path = default_grammar_path;

  char *scope_name = NULL;
  bool html = false;

  char *default_theme_path = "./samples/monokai.json";
  char *theme_path = default_theme_path;

  char *default_extensions_path = "./tests/data/extensions";
  char *extensions_path = default_extensions_path;

  if (argc > 1) {
    path = argv[argc - 1];
  }
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-c") == 0 ||
        strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-z") == 0) {
      _debug = argv[i][1];
    }
    if (strcmp(argv[i], "-m") == 0) {
      html = true;
    }
    if (strcmp(argv[i - 1], "-t") == 0) {
      theme_path = argv[i];
    }
    if (strcmp(argv[i - 1], "-s") == 0) {
      scope_name = argv[i];
    }
    if (strcmp(argv[i - 1], "-l") == 0) {
      grammar_path = argv[i];
    }
    if (strcmp(argv[i - 1], "-x") == 0) {
      extensions_path = argv[i];
    }
  }

  tx_read_package_dir(extensions_path);

  TxThemeNode *theme = txn_load_theme(theme_path);
  if (!theme) {
    goto exit;
  }

  TxSyntaxNode *root = NULL;
  if (scope_name) {
    root = tx_syntax_from_scope(scope_name);
    if (!root) {
      root = tx_syntax_from_path(scope_name);
    }
    if (!root) {
      goto exit;
    }

  } else {
    root = grammar_path ? txn_load_syntax(grammar_path)
                        : tx_syntax_from_path(path);

    if (!root) {
      goto exit;
    }

    if (grammar_path) {
      TxNode *scope = txn_get(root, "scopeName");
      txn_set(tx_global_repository(),
              scope ? scope->string_value : "source.xxx", root);
    }
  }

  // dump(theme, 0);

  TX_TIMER_END

  if (!html) {
    printf("grammar loaded at %fsecs\n", _cpu_time_used);
  }

  TX_TIMER_RESET

  TxParserStateNode *psn = txn_new_parser_state();

  TxParserState stack;
  tx_init_parser_state(&stack, txn_syntax_value(root));

  TxParseProcessor processor;
  switch (_debug) {
  case 'd':
    tx_init_processor(&processor, TxProcessorTypeCollectAndDump);
    break;
  case 'c':
    tx_init_processor(&processor, TxProcessorTypeCollect);
    break;
  case 'r':
    tx_init_processor(&processor, TxProcessorTypeCollectAndRender);
    processor.render_html = html;
    break;
  case 'z':
  default:
    tx_init_processor(&processor, TxProcessorTypeDump);
    break;
  }
  processor.theme = txn_theme_value(theme);

  // dump(root, 0);
  if (html) {
    printf("<html><body style='background:#303030'>");
  }

  char temp[TX_MAX_LINE_LENGTH];
  FILE *fp = fopen(path, "r");
  if (fp) {
    while (!feof(fp)) {
      strcpy(temp, "");
      fgets(temp, TX_MAX_LINE_LENGTH, fp);
      int len = strlen(temp);
      // printf("%s", temp);
      txn_parser_state_unserialize(psn, &stack);
      tx_parse_line(temp, temp + len, &stack, &processor);
      txn_parser_state_serialize(psn, &stack);
    }
    fclose(fp);
  }

  TX_TIMER_END
  if (!html) {
    printf("\nfile %s parsed at %fsecs\n", path, _cpu_time_used);
  }

  // dump(tx_global_repository(), 0);

exit:
  if (theme) {
    TxNode *child = theme->theme.unresolved_scopes->first_child;
    while (child) {
      printf("%s\n", child->string_value);
      child = child->next_sibling;
    }
    txn_free(theme);
  }

  txn_free(psn);

  tx_shutdown();

  if (html) {
    printf("</body></html>");
  } else {
    tx_stats();
    printf("\x1b[0");
  }

  return 0;
}