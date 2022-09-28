#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool _debug = false;

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  tx_initialize();

  char *default_path = "./samples/simple.c";
  char *path = default_path;

  char *default_grammar_path = NULL;
  char *grammar_path = default_grammar_path;

  char *default_theme_path = "./samples/monokai.json";
  char *theme_path = default_theme_path;

  char *default_extensions_path = "/home/iceman/.editor/extensions";
  char *extensions_path = default_extensions_path;

  if (argc > 1) {
    path = argv[argc - 1];
  }
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      _debug = true;
    }
    if (strcmp(argv[i - 1], "-t") == 0) {
      theme_path = argv[i];
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

  TxSyntaxNode *root =
      grammar_path ? txn_load_syntax(grammar_path) : tx_syntax_from_path(path);
  if (grammar_path) {
    txn_set(tx_global_repository(), "source.x", root);
  }

  // dump(theme, 0);

  TX_TIMER_END
  printf("grammar loaded at %fsecs\n", _cpu_time_used);

  TX_TIMER_RESET

  TxParserState stack;
  tx_init_parser_state(&stack, txn_syntax_value(root));

  TxParseProcessor processor;
  // tx_init_processor(&processor, TxProcessorTypeDump);
  // tx_init_processor(&processor, TxProcessorTypeCollect);
  tx_init_processor(&processor, _debug ? TxProcessorTypeCollectAndDump
                                       : TxProcessorTypeCollectAndRender);
  processor.theme = txn_theme_value(theme);

  // dump(root, 0);

  char temp[1024];
  FILE *fp = fopen(path, "r");
  while (!feof(fp)) {
    strcpy(temp, "");
    fgets(temp, 1024, fp);

    int len = strlen(temp);
    // printf("%s", temp);
    tx_parse_line(temp, temp + len + 1, &stack, &processor);
  }
  fclose(fp);

  TX_TIMER_END
  printf("\nfile %s parsed at %fsecs\n", path, _cpu_time_used);

  // dump(tx_global_repository(), 0);

  txn_free(theme);

  tx_shutdown();
  tx_stats();

  printf("\x1b[0");
  return 0;
}