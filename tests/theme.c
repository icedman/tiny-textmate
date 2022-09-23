#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  tx_initialize();

  TX_TIMER_BEGIN

  char *default_path = "./tests/dracula.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxThemeNode *thm = txn_load_theme(path);

  dump(thm, 0);

  txn_free(thm);
  tx_shutdown();
  tx_stats();

  TX_TIMER_END
}