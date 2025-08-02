#include "textmate.h"
#include <stdio.h>
#include <string.h>

#define _PRINT_BUFFER_RANGE(B, S, E)                                           \
  {                                                                            \
    printf("%s", tx_extract_buffer_range(B, S, E));                            \
  }

//------------------
// null processor
//------------------
static void null_line_start(TxParseProcessor *self, char *buffer_start,
                            char *buffer_end) {}

static void null_line_end(TxParseProcessor *self) {}

static void null_open_tag(TxParseProcessor *self, TxMatch *state) {}

static void null_close_tag(TxParseProcessor *self, TxMatch *state) {}

static void null_capture(TxParseProcessor *self, TxMatch *state) {}

//------------------
// dump processor
//------------------
static void dump_line_start(TxParseProcessor *self, char *buffer_start,
                            char *buffer_end) {
  printf("---------------------\n");
  _PRINT_BUFFER_RANGE(buffer_start, 0, buffer_end - buffer_start)
  printf("\n");
}

static void dump_line_end(TxParseProcessor *self) {}

static void dump_open_tag(TxParseProcessor *self, TxMatch *state) {
  _BEGIN_COLOR(255, 255, 0)
  printf("{{{{ ");
  _PRINT_BUFFER_RANGE(state->matches[0].buffer, state->matches[0].start,
                      state->matches[0].end);
  printf("\n");

  _END_FORMAT
  printf("[%s] (%d-%d) while:%x\n", state->matches[0].scope,
         state->matches[0].start, state->matches[0].end,
         state->syntax->rx_while);
}

static void dump_close_tag(TxParseProcessor *self, TxMatch *state) {
  _BEGIN_COLOR(255, 255, 0)
  _PRINT_BUFFER_RANGE(state->matches[0].buffer, state->matches[0].start,
                      state->matches[0].end);
  printf(" }}}}\n");
  _END_FORMAT
  printf(" [%s] (%d-%d)\n", state->matches[0].scope, state->matches[0].start,
         state->matches[0].end);
}

static void dump_capture(TxParseProcessor *self, TxMatch *state) {
  for (int i = 0; i < state->size; i++) {
    if (!state->matches[i].scope[0])
      continue;
    // if (state->matches[i].start < 0 || state->matches[i].end < 0) continue;
    _BEGIN_COLOR(0, 255, 255)
    _PRINT_BUFFER_RANGE(state->matches[i].buffer, state->matches[i].start,
                        state->matches[i].end);
    _END_FORMAT
    printf(" :: [%s] %d (%d-%d)\n", state->matches[i].scope, i,
           state->matches[i].start, state->matches[i].end);
  }
}

//------------------
// collect processor
//------------------
static void collect_line_start(TxParseProcessor *self, char *buffer_start,
                               char *buffer_end) {
  self->buffer_start = buffer_start;
  self->buffer_end = buffer_end;
  tx_init_parser_state(&self->line_parser_state, NULL);

  // prior lines states is copied to stack and range is set to span the entire
  // line
  for (int i = 0; i < self->parser_state->size; i++) {
    TxMatch *match = &self->parser_state->states[i];
    tx_state_push(&self->line_parser_state, match);
    self->line_parser_state.states[i].matches[0].start = 0;
    self->line_parser_state.states[i].matches[0].end =
        buffer_end - buffer_start;
  }
}

static void collect_open_tag(TxParseProcessor *self, TxMatch *state) {
  tx_state_push(&self->line_parser_state, state);
  // at open_tag - the range is set to span until end of line - unless corrected
  // by a close tag
  tx_state_top(&self->line_parser_state)->matches[0].end =
      self->buffer_end - self->buffer_start;
}

static void collect_close_tag(TxParseProcessor *self, TxMatch *state) {
  TxParserState *line_parser_state = &self->line_parser_state;
  // at close tag - the range of a begin match is corrected
  for (int j = line_parser_state->size; j > 0; j--) {
    TxMatch *m = &line_parser_state->states[j - 1];
    if (m->syntax == state->syntax) {
      m->matches[0].end = state->matches[0].end;
      break;
    }
  }
}

static void collect_capture(TxParseProcessor *self, TxMatch *state) {
  tx_state_push(&self->line_parser_state, state);
}

static void collect_dump_line_end(TxParseProcessor *self) {
  TxParserState *line_parser_state = &self->line_parser_state;

  for (int j = 0; j < line_parser_state->size; j++) {
    TxMatch *state = &line_parser_state->states[j];
    for (int i = 0; i < state->size; i++) {
      if (!state->matches[i].scope[0])
        continue;
      if (state->matches[i].start < 0 || state->matches[i].end < 0 ||
          !state->matches[i].buffer)
        continue;
      _BEGIN_COLOR(0, 255, 255)
      _PRINT_BUFFER_RANGE(state->matches[i].buffer, state->matches[i].start,
                          state->matches[i].end);
      _END_FORMAT
      printf(" :: [%s] %d (%ld-%ld)\n", state->matches[i].scope, i,
             state->matches[i].start, state->matches[i].end);
    }
  }
}

static void collect_render_line_end(TxParseProcessor *self) {
  TxParserState *line_parser_state = &self->line_parser_state;

  if (self->render_html) {
    printf("<span>");
  }

  int prev_color = -1;
  int32_t fg = 0xf0f0f0;
  if (self->theme) {
    TxStyleSpan style;
    if (tx_style_from_scope("foreground", self->theme, &style)) {
      fg = style.font_style.fg;
    } else if (tx_style_from_scope("editor.foreground", self->theme, &style)) {
      fg = style.font_style.fg;
    }
  }

  char *c = self->buffer_start;
  int idx = 0;
  TxStyleSpan style;
  while (c < self->buffer_end) {

    style.font_style.fg = fg;

    TxMatchRange *match_range = NULL;
    for (int j = 0; j < line_parser_state->size; j++) {
      TxMatch *state = &line_parser_state->states[j];
      for (int i = 0; i < state->size; i++) {
        if (!state->matches[i].scope[0])
          continue;

        TxMatchRange *range = &state->matches[i];

        if (state->matches[i].start < 0 || state->matches[i].end < 0)
          continue;
        if (state->matches[i].start <= idx && idx < state->matches[i].end) {
          TxStyleSpan _style;
          if (self->theme &&
              !tx_style_from_scope(range->scope, self->theme, &_style)) {
            range = NULL;
          }
          if (range) {
            // printf(">>> [%s] %d (%ld-%ld)\n", range->scope, idx,
            //    range->start, range->end);
            match_range = range;
            style = _style;
          }
        }
      }
    }

    if (self->theme) {
      if (prev_color != style.font_style.fg) {
        uint32_t rgb[] = {255, 255, 255};
        if (txt_color_to_rgb(style.font_style.fg, rgb)) {
          if (self->render_html) {
            if (*c != ' ' && *c != '\t') {
              printf("</span><span style='color: rgb(%d,%d,%d)'>", rgb[0],
                     rgb[1], rgb[2]);
            }
          } else {
            _BEGIN_COLOR(rgb[0], rgb[1], rgb[2])
          }
        }
        prev_color = style.font_style.fg;
      }
    }

    if (self->render_html) {
      switch (*c) {
      case ' ':
        printf("&nbsp;");
        break;
      case '\t':
        printf("&nbsp;&nbsp;&nbsp;&nbsp;");
        break;
      case '<':
        printf("&lt;");
        break;
      default:
        printf("%c", *c);
        break;
      }
    } else {
      printf("%c", *c);
    }

    c++;
    idx++;
  }

  if (self->render_html) {
    printf("</span><br/>\n");
  } else {
    _END_FORMAT
  }
}

static void collect_style_line_end(TxParseProcessor *self) {
  TxParserState *line_parser_state = &self->line_parser_state;

  int len = self->buffer_end - self->buffer_start;
  int style_idx = 0;
  TxStyleSpan default_style;

  if (self->theme) {
    if (tx_style_from_scope("foreground", self->theme, &default_style)) {
      //
    } else if (tx_style_from_scope("editor.foreground", self->theme,
                                   &default_style)) {
      //
    }
  }

  char *c = self->buffer_start;
  int idx = 0;
  while (c < self->buffer_end) {

    TxStyleSpan style = default_style;
    style.start = idx;
    style.end = len;
    TxMatchRange *match_range = NULL;

    for (int j = 0; j < line_parser_state->size; j++) {
      TxMatch *state = &line_parser_state->states[j];
      for (int i = 0; i < state->size; i++) {
        if (!state->matches[i].scope[0])
          continue;

        TxMatchRange *range = &state->matches[i];

        if (state->matches[i].start < 0 || state->matches[i].end < 0)
          continue;
        if (state->matches[i].start <= idx && idx < state->matches[i].end) {
          TxStyleSpan _style;
          if (self->theme &&
              !tx_style_from_scope(range->scope, self->theme, &_style)) {
            range = NULL;
          }
          if (range) {
            match_range = range;
            style = _style;
          }
        }
      }
    }

    style.start = idx;
    style.end = idx + 1;
    if (style_idx < TX_MAX_STYLE_SPANS) {
      if (self->theme || style_idx == 0) {
        if (style_idx == 0) {
          self->line_styles[style_idx++] = style;
        } else {
          if (self->line_styles[style_idx - 1].font_style.fg ==
              style.font_style.fg) {
            self->line_styles[style_idx - 1].end = idx + 1;
          } else {
            self->line_styles[style_idx++] = style;
          }
        }
      }
    }

    c++;
    idx++;
  }

  self->line_styles[style_idx - 1].end = len;
  self->line_styles_size = style_idx;

  // for(int i=0; i<self->line_styles_size; i++) {
  //   printf("(%d-%d) fg:%x\n", self->line_styles[i].start,
  //       self->line_styles[i].end, self->line_styles[i].fg);
  // }
}

//------------------

void tx_init_processor(TxParseProcessor *processor, TxProcessorType type) {
  memset(processor, 0, sizeof(TxParseProcessor));
  switch (type) {
  case TxProcessorTypeDump:
    processor->line_start = dump_line_start;
    processor->line_end = dump_line_end;
    processor->open_tag = dump_open_tag;
    processor->close_tag = dump_close_tag;
    processor->capture = dump_capture;
    break;
  case TxProcessorTypeCollect:
  case TxProcessorTypeCollectAndDump:
  case TxProcessorTypeCollectAndRender:
  case TxProcessorTypeCollectAndStyle:
    processor->line_start = collect_line_start;
    processor->line_end = null_line_end;
    processor->open_tag = collect_open_tag;
    processor->close_tag = collect_close_tag;
    processor->capture = collect_capture;
    break;
  default:
    processor->line_start = null_line_start;
    processor->line_end = null_line_end;
    processor->open_tag = null_open_tag;
    processor->close_tag = null_close_tag;
    processor->capture = null_capture;
    break;
  }

  switch (type) {
  case TxProcessorTypeCollectAndDump:
    processor->line_end = collect_dump_line_end;
    break;
  case TxProcessorTypeCollectAndRender:
    processor->line_end = collect_render_line_end;
    break;
  case TxProcessorTypeCollectAndStyle:
    processor->line_end = collect_style_line_end;
    break;
  }
}
