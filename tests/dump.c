#include "textmate.h"

#include <stdio.h>

#define NAME_LENGTH 256

void dump(TxNode *n, int level) {
  // if (level > 3)
  //   return;
  char indent[64] = "";
  for (int i = 0; i < level * 2; i++) {
    indent[i] = ' ';
    indent[i + 1] = 0;
  }

  char name[NAME_LENGTH] = "";
  if (n->name) {
    sprintf(name, "%s: ", n->name);
  }
  if (n->string_value && !n->data) {
    printf("%s%s%s\n", indent, name, txn_string_value(n));
  }

  switch (n->type) {
  case TxTypeNumber: {
    printf("%s%s%d\n", indent, name, txn_number_value(n));
    break;
  }
  case TxTypeString:
    break;
  // case TxTypeString: {
  //   printf("%s%s%s\n", indent, name, txn_string_value(n));
  //   break;
  // }
  case TxTypeObject:
  case TxTypeArray: {
    TxNode *c = n->first_child;
    if (n->name) {
      printf("%s%s\n", indent, name);
    }
    while (c) {
      dump(c, level + 1);
      c = c->next_sibling;
    }
    break;
  }
  default:
    printf("%s?\n", indent);
    break;
  }
  // printf("%s---\n", indent);
}