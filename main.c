#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool _debug = false;

void dump(TxNode *n, int level);

static void open_tag(TxProcessor *self, TxStateStack *stack) {
  TxState *top = txs_top(stack);
  TxState state;
  memcpy(&state, top, sizeof(TxState));
  state.matches[0].end = self->length;
  txs_push(&self->state_stack, &state);

  if (_debug) {
    printf("{{{<%s> (%d-%d)\n", top->matches[0].expanded,
           top->matches[0].start + top->offset,
           top->matches[0].end + top->offset);
  }
}

static void close_tag(TxProcessor *self, TxStateStack *stack) {
  TxState *top = txs_top(stack);

  for (int i = self->state_stack.size; i > 0; i--) {
    TxState *state = &self->state_stack.states[i - 1];
    for (int j = state->count; j > 0; j--) {
      if (strcmp(state->matches[j - 1].expanded, top->matches[0].expanded) ==
          0) {
        state->matches[j - 1].end = top->offset + top->matches[0].end;
        // break;
      }
    }
  }

  if (_debug) {
    printf("<%s> (%d-%d) }}}\n", top->matches[0].expanded,
           top->matches[0].start + top->offset,
           top->matches[0].end + top->offset);
  }
}

static void capture(TxProcessor *self, TxState *match, TxCaptureList captures) {
  TxState state;
  memcpy(&state, match, sizeof(TxState));
  memcpy(&state.matches, captures, sizeof(TxCaptureList));
  txs_push(&self->state_stack, &state);
}

static void line_start(TxProcessor *self, TxStateStack *stack, char_u *buffer,
                       size_t len) {
  txs_init_stack(&self->state_stack);
  self->buffer = buffer;
  self->length = len;

  TxState *top = txs_top(stack);
  TxState state;
  memcpy(&state, top, sizeof(TxState));
  state.matches[0].start = 0;
  state.matches[0].end = len;
  txs_push(&self->state_stack, &state);
}

static void line_end(TxProcessor *self, TxStateStack *stack) {
  TxStateStack *state_stack = &self->state_stack;
  TxTheme *theme = (TxTheme *)self->data;

  int32_t clr = -1;
  int32_t current_clr = -1;
  for (int k = 0; k < self->length; k++) {
    TxCapture *_c = NULL;
    clr = -1;
    for (int i = 0; i < state_stack->size; i++) {
      TxState *state = &state_stack->states[i];
      for (int j = 0; j < state->count; j++) {
        int capture_idx = j;
        if (capture_idx >= TS_MAX_CAPTURES)
          break;
        TxCapture *capture = &state->matches[capture_idx];
        if (strlen(capture->expanded) == 0 || capture->start == capture->end)
          continue;
        if (capture->start + state->offset <= k &&
            k < capture->end + state->offset) {
          _c = capture;
          TxStyleSpan span;
          if (txt_style_from_scope(_c->expanded, theme, &span)) {
            clr = span.fg;
          }
        }
      }
    }
    if (clr == -1) {
      clr = txt_make_color(200, 200, 200);
    }

    if (current_clr != clr) {
      uint32_t rgb[3] = {200, 200, 200};
      txt_color_to_rgb(clr, rgb);
      printf("\x1b[38;2;%d;%d;%dm", rgb[0], rgb[1], rgb[2]);
      current_clr = clr;
    }
    printf("%c", self->buffer[k]);
  }

  printf("\x1b[0m");
}

static void line_start_(TxProcessor *self, TxStateStack *stack, char_u *buffer,
                        size_t len) {
  char temp[1024] = "";
  memcpy(temp, buffer, sizeof(char_u) * len);
  printf("%s\n", temp);

  line_start(self, stack, buffer, len);
}

static void line_end_(TxProcessor *self, TxStateStack *stack) {
  TxStateStack *state_stack = &self->state_stack;
  TxTheme *theme = (TxTheme *)self->data;

  for (int i = 0; i < state_stack->size; i++) {
    TxState *state = &state_stack->states[i];
    for (int j = 0; j < state->count; j++) {
      int capture_idx = j;
      if (capture_idx >= TS_MAX_CAPTURES)
        break;
      TxCapture *capture = &state->matches[capture_idx];
      // if (strlen(capture->expanded) == 0 || capture->start == capture->end)
      //   continue;

      TxStyleSpan span;
      uint32_t rgb[3] = {255, 255, 255};
      if (txt_style_from_scope(capture->expanded, theme, &span)) {
        txt_color_to_rgb(span.fg, rgb);
      }

      printf("\x1b[38;2;%d;%d;%dm", rgb[0], rgb[1], rgb[2]);
      printf("%d (%ld-%ld) [%s]\n", capture_idx, capture->start + state->offset,
             capture->end + state->offset, capture->expanded);
      printf("\x1b[0m");
    }
  }
}

int main(int argc, char **argv) {
  TX_TIMER_BEGIN

  TxProcessor processor;
  txp_init_processor(&processor);
  processor.line_start = line_start;
  processor.line_end = line_end;
  processor.open_tag = open_tag;
  processor.close_tag = close_tag;
  processor.capture = capture;

  tx_initialize();

  char *default_path = "./samples/simple.c";
  char *path = default_path;

  char *default_grammar_path = NULL;
  char *grammar_path = default_grammar_path;

  char *default_theme_path = "./samples/monokai.json";
  char *theme_path = default_theme_path;

  char *default_extensions_path = "/home/iceman/.editor/extensions";
  char *extensions_path = default_extensions_path;

  if (argc > 1) {
    path = argv[argc - 1];
  }
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      processor.line_start = line_start_;
      processor.line_end = line_end_;
      _debug = true;
    }
    if (strcmp(argv[i - 1], "-t") == 0) {
      theme_path = argv[i];
    }
    if (strcmp(argv[i - 1], "-l") == 0) {
      grammar_path = argv[i];
    }
    if (strcmp(argv[i - 1], "-x") == 0) {
      extensions_path = argv[i];
    }
  }

  tx_read_package_dir(extensions_path);

  TxThemeNode *theme = txn_load_theme(theme_path);
  processor.data = txn_theme_value(theme);

  TxSyntaxNode *root =
      grammar_path ? txn_load_syntax(grammar_path) : tx_syntax_from_path(path);
  if (grammar_path) {
    txn_set(tx_global_repository(), "source.x", root);
  }

  // dump(theme, 0);

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

    // char nl[] = "\n";
    // tx_parse_line(nl, nl + 1, &stack, &processor);
  }
  fclose(fp);

  TX_TIMER_END
  printf("\nfile %s parsed at %fsecs\n", path, _cpu_time_used);

  // dump(tx_global_repository(), 0);

  txn_free(theme);

  tx_shutdown();
  tx_stats();

  printf("\x1b[0");
  return 0;
}