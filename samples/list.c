#include "textmate.h"

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  TxNode *n = txn_new_array();

  txn_push(n, txn_new_number(0));
  txn_push(n, txn_new_number(1));
  txn_push(n, txn_new_number(2));
  txn_push(n, txn_new_string("hello world"));

  txn_free(txn_pop(n));

  TxNode *dn = txn_new_object();
  txn_set(n, "hello", txn_new_number(123));
  txn_set(n, "world", txn_new_number(456));
  txn_push(n, dn);

  TxNode *dn2 = txn_new_object();
  txn_set(dn2, "hello", txn_new_number(123));
  txn_set(dn2, "world", txn_new_number(456));
  txn_set(dn, "sub", dn2);

  txn_free(txn_pop(n));
  // tx_stats();

  dump(n, 0);

  txn_free(n);
  tx_stats();

  return 0;
}