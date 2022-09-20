#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  tx_initialize();

  char *default_path = "./tests/c.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxNode* global_repository = tx_global_repository();
  TxSyntaxNode *root = txn_load_syntax(path);
  txn_set(global_repository, "source.c", root);

  TX_TIMER_END
  printf("grammar loaded at %fsecs\n", _cpu_time_used);

  TxStateStack stack;
  TxState state;
  txs_init_stack(&stack);
  txs_init_state(&state);
  state.syntax = txn_syntax_value(root);
  txs_push(&stack, &state);

  // dump(root, 0);

  tx_shutdown();
  tx_stats();

  printf("sizeof TxStateStack %ld\n", sizeof(TxStateStack));

  return 0;
}