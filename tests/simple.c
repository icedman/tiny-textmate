// comment line
void dump(TxNode *n, int level);

/*
this is a comment block
*/
#include "textmate.h"
int main(int argc, char **argv) {
  for (int i = 0; i < 200; i++) {
    printf("%d %d\n", i, i);
    printf("%d %d\n",
      i,
      i);
  }

  printf("%d %d\n", i, i);
  return 0;
}

#include "textmate.h"
