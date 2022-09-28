#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  tx_initialize();

  TxSyntaxNode *root = txn_load_json("./tests/data/main.c.spec.json");
  // TxSyntaxNode *root = txn_load_json("./samples/c.json");

  TX_TIMER_END

  dump(root, 0);

  txn_free(root);

  tx_shutdown();
  tx_stats();

  return 0;
}