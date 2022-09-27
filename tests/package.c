#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int test_single_package(int argc, char **argv) {
  TX_TIMER_BEGIN

  char *default_path = "./samples/cpp.package.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxPackageNode *root = txn_load_package(path);

  TX_TIMER_END
  printf("package loaded at %fsecs\n", _cpu_time_used);

  dump(root, 0);

  txn_free(root);
  return 0;
}

int test_packages(int argc, char **argv) {
  TX_TIMER_BEGIN

  char *default_path = "/home/iceman/.editor/extensions/";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  // todo leak
  tx_read_package_dir(path);
  dump(tx_global_packages(), 0);
  // tx_syntax_from_path("/x/Makefile");
  // tx_syntax_from_path("/x/test.xc");
  // tx_syntax_from_path("/x/test.c");
  // tx_syntax_from_scope("source.c");
  // dump(tx_global_packages(), 0);

  printf("packages loaded at %fsecs\n", _cpu_time_used);
  TX_TIMER_END
  return 0;
}

int main(int argc, char **argv) {
  tx_initialize();

  // test_single_package(argc, argv);
  test_packages(argc, argv);

  tx_shutdown();
  tx_stats();
}