#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

void line_begin(struct TxProcessor *self, TxStateStack *stack)
{
  printf("--------------------------------------------\n");
}

void open_tag(struct TxProcessor *self, TxStateStack *stack)
{}

void end_tag(struct TxProcessor *self, TxStateStack *stack)
{}

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  TxProcessor processor;
  txp_init_processor(&processor);
  processor.line_begin = line_begin;
  processor.open_tag = open_tag;
  processor.end_tag = end_tag;

  tx_initialize();

  char *default_path = "./tests/simple.c";
  char *path = default_path;

  char *default_grammar_path = "./tests/c.json";
  char *grammar_path = default_grammar_path;

  if (argc > 1) {
    path = argv[1];
  }
  if (argc > 2) {
    grammar_path = argv[2];
  }

  TxSyntaxNode *root = txn_load_syntax(grammar_path);
  // TxSyntaxNode *root = txn_load_syntax("./tests/cpp.json");
  txn_set(tx_global_repository(), "source.c", root);

  TX_TIMER_END
  printf("grammar loaded at %fsecs\n", _cpu_time_used);

  TX_TIMER_RESET

  TxStateStack stack;
  TxState state;
  txs_init_stack(&stack);
  txs_init_state(&state);
  state.syntax = txn_syntax_value(root);
  txs_push(&stack, &state);

  // dump(root, 0);

  char temp[1024];
  FILE *fp = fopen(path, "r");
  while (!feof(fp)) {
    fgets(temp, 1024, fp);

    int len = strlen(temp);
    printf("%s", temp);
    tx_parse_line(temp, temp + len, &stack, &processor);

    // temp[0] = "\n";
    // tx_parse_line(temp, temp + 1, &stack, NULL);
  }
  fclose(fp);

  TX_TIMER_END
  printf("file %s parsed at %fsecs\n", path, _cpu_time_used);

  // dump(tx_global_repository(), 0);

  tx_shutdown();
  tx_stats();
  return 0;
}