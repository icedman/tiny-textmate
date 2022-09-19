#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int test_single_package(int argc, char **argv) {
  TX_TIMER_BEGIN

  char *default_path = "./tests/cpp.package.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxPackageNode *root = txn_load_package(path);

  TX_TIMER_END
  printf("package loaded at %fsecs\n", _cpu_time_used);

  dump(root, 0);

  txn_free(root);
  tx_stats();

  tx_read_package_dir("/home/iceman/.editor/extensions/");
  return 0;
}

int test_packages(int argc, char **argv) {
  TX_TIMER_BEGIN

  char *default_path = "/home/iceman/.editor/extensions/";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  tx_read_package_dir(path);

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