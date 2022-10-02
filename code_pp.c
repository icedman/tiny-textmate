#include "cJSON.h"
#include "textmate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static char _debug = 'r';

int main(int argc, char **argv) {

  tx_initialize();

  char *default_grammar_path = NULL;
  char *grammar_path = default_grammar_path;

  char *scope_name = NULL;
  bool html = false;

  char *default_theme_path = "./samples/monokai.json";
  char *theme_path = default_theme_path;

  char *default_extensions_path = "./tests/data/extensions";
  char *extensions_path = default_extensions_path;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-c") == 0 ||
        strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-z") == 0) {
      _debug = argv[i][1];
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
    if (strcmp(argv[i - 1], "-h") == 0) {
      html = true;
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
  } else {
    root = grammar_path ? txn_load_syntax(grammar_path)
                        : tx_syntax_from_path("code.c");

    if (!root) {
      goto exit;
    }

    if (grammar_path) {
      TxNode *scope = txn_get(root, "scopeName");
      txn_set(tx_global_repository(),
              scope ? scope->string_value : "source.xxx", root);
    }
  }

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

  char line[TX_MAX_LINE_LENGTH];
  char *p = line;
  char c;
  int wait = 500;

  while (1) {
    if (!read(STDIN_FILENO, &c, 1))
      break;
    *p++ = c;
    if (c == '\n' || (p - line + 1 > TX_MAX_LINE_LENGTH)) {
      tx_parse_line(line, p, &stack, &processor);
      p = line;
    }
  }

  if (p != line) {
    tx_parse_line(line, p, &stack, &processor);
  }

exit:
  if (theme) {
    txn_free(theme);
  }

  tx_shutdown();
  // tx_stats();

  printf("\x1b[0\n");
  return 0;
}