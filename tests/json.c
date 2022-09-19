#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  tx_initialize();

  TxSyntaxNode *root = txn_load_syntax("./c.json");

  TX_TIMER_END
  
  dump(root, 0);
  printf("grammar loaded at %fsecs\n", _cpu_time_used);

  txn_free(root);
  tx_stats();

  return 0;
}