#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void dump(TxNode *n, int level);

void dump_match(TxState *state, TxCaptureList matches) {
  char_u *name = state->syntax->scope_name;
  if (!name) {
    name = state->syntax->content_name;
  }
  if (!name) {
    name = state->syntax->name;
  }
  if (name) {
    char_u expanded[TS_SCOPE_NAME_LENGTH] = "";
    tx_expand_name(name, expanded, matches);
    printf("<%ld-%ld> [%s]\n", matches[0].start, matches[0].end, expanded);
  }
}

static void capture(TxProcessor *self, TxState *match) {
  dump_match(match, &match->matches);
  txs_push(&self->line_state, match);
}

static void line_start(TxProcessor *self, TxStateStack *stack, char_u *buffer,
                       size_t len) {
  txs_init_stack(&self->line_state);

#define TZ 1024
  if (len > TZ) {
    len = TZ;
  }
  char temp[TZ];
  strncpy(temp, buffer, len);
  temp[len] = 0;
  printf("--------------------------------------------\n%s", temp);

  // capture(self, txs_top(stack));
}

static void line_end(TxProcessor *self, TxStateStack *stack, char_u *buffer,
                     size_t len) {
  // TxProcessor *proc = (TxProcessor*)self;
  // TxStateStack *line_state = &self->line_state;

  // for(int i = 0; i< line_state->size; i++) {
  //   TxState *state = &line_state->states[i];
  //   for (int j = 1; j < state->count; j++) {
  //     int capture_idx = j;
  //     if (capture_idx >= TS_MAX_CAPTURES)
  //       break;
  //     TxCapture *capture = &state->matches[capture_idx - 1];
  //     if (strlen(capture->expanded) == 0 || capture->start == capture->end)
  //     continue; printf("(%ld-%ld) [%s]\n", capture->start, capture->end,
  //     capture->expanded);
  //   }
  // }
}

static void open_tag(TxProcessor *self, TxState *match) {
  capture(self, match);
}

static void end_tag(TxProcessor *self, TxState *match) { capture(self, match); }

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  TxProcessor processor;
  txp_init_processor(&processor);
  processor.line_start = line_start;
  // processor.line_end = line_end;
  // processor.open_tag = capture;
  // processor.end_tag = capture;
  // processor.capture = capture;

  tx_initialize();

  char *default_path = "./tests/simple.c";
  char *path = default_path;

  char *default_grammar_path = "./tests/c.json";
  char *grammar_path = default_grammar_path;

  if (argc > 1) {
    path = argv[1];
  }
  if (argc > 2) {
    grammar_path = argv[2];
  }

  TxThemeNode *theme = txn_load_theme("./tests/dracula.json");

  TxSyntaxNode *root = txn_load_syntax(grammar_path);
  // TxSyntaxNode *root = txn_load_syntax("./tests/cpp.json");
  txn_set(tx_global_repository(), "source.c", root);

  // dump(root, 0);

  TX_TIMER_END
  printf("grammar loaded at %fsecs\n", _cpu_time_used);

  TX_TIMER_RESET

  TxStateStack stack;
  TxState state;
  txs_init_stack(&stack);
  txs_init_state(&state);
  state.syntax = txn_syntax_value(root);
  txs_push(&stack, &state);

  // dump(root, 0);

  char temp[1024];
  FILE *fp = fopen(path, "r");
  while (!feof(fp)) {
    fgets(temp, 1024, fp);

    int len = strlen(temp);
    // printf("%s", temp);
    tx_parse_line(temp, temp + len + 1, &stack, &processor);
    strcpy(temp, "");

  }
  fclose(fp);

  TX_TIMER_END
  printf("file %s parsed at %fsecs\n", path, _cpu_time_used);

  // dump(tx_global_repository(), 0);

  txn_free(theme);

  tx_shutdown();
  tx_stats();
  return 0;
}