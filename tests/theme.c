#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

int main(int argc, char **argv) {
  tx_initialize();

  TX_TIMER_BEGIN

  char *default_path = "./samples/dracula.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxThemeNode *thm = txn_load_theme(path);
  TxTheme *t = txn_theme_value(thm);

  const char *scopes[] = {
    "meta.paragraph.markdown",
    "meta.embedded.block.frontmatter",
    "meta.flow-sequence.yaml",
    "meta.paragraph.markdown",
    "markup.list.unnumbered.markdown",
    "markup.list.numbered.markdown",
    0
  };

  for(int i=0; ;i++) {
    char *s = scopes[i];
    if (!s) break;
    printf("-------------\n%s\n", s);
    TxStyleSpan style;
    tx_style_from_scope(s, t, &style);
  }

  dump(thm, 0);

  txn_free(thm);
  tx_shutdown();
  tx_stats();

  TX_TIMER_END
}