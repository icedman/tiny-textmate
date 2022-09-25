#include "textmate.h"
#include <stdio.h>
#include <string.h>

static void _dump_syntax_node(TxNode *node) {
  if (node->name) {
    printf("%s[%d]\\", node->name, node->index);
  }
  if (node->parent) {
    _dump_syntax_node(node->parent);
  }
}

static void dump_syntax_node(TxNode *node) {
  _dump_syntax_node(node);
  printf("\n");
}

static void expand_match(TxState *state) {
  char_u *name = state->syntax->scope_name;
  if (!name) {
    name = state->syntax->content_name;
  }
  if (!name) {
    name = state->syntax->name;
  }
  if (name) {
    char_u expanded[TS_SCOPE_NAME_LENGTH] = "";
    tx_expand_name(name, expanded, state->matches);
    // printf("<%ld-%ld> [%s]\n", state->matches[0].start + state->offset,
    //        state->matches[0].end + state->offset, expanded);
  }
}

static void expand_captures(TxState *state, TxCaptureList target_capture,
                            TxCaptureList source_capture,
                            TxProcessor *processor) {
  for (int j = 0; j < state->count; j++) {
    int capture_idx = j;
    if (capture_idx >= TS_MAX_CAPTURES)
      break;

    TxCapture *capture = &target_capture[capture_idx];
    if (!capture->scope) {
      continue;
    }
    if (capture->start == capture->end) {
      continue;
    }

    if (!tx_expand_name(capture->scope, capture->expanded, source_capture)) {
      continue;
    }

    // printf("%d %d (%ld-%ld) [%s]\n", j, state->offset,
    //        capture->start + state->offset, capture->end + state->offset,
    //        capture->expanded);
  }

  // if (processor && processor->capture) {
  //   processor->capture(processor, state, target_capture);
  // }
  // expand_match(state);
}

TxSyntax *txn_syntax_value_proxy(TxSyntaxNode *node) {
  TxSyntax *syntax = txn_syntax_value(node);

  if (syntax) {
    if (!syntax->include && syntax->include_external) {
      syntax->include_external = false;
    }

    if (syntax->include) {
      TxSyntaxNode *include_node = syntax->include;
      syntax = txn_syntax_value_proxy(include_node);
    }
  }

  return syntax;
}

static bool find_match(char_u *buffer_start, char_u *buffer_end, regex_t *regex,
                       TxState *state, char_u *pattern) {
  bool found = false;

  OnigRegion *region = onig_region_new();
  int r;
  unsigned char *start, *range, *end;
  unsigned int onig_options = ONIG_OPTION_NONE;

  int count = 0;
  UChar *str = (UChar *)buffer_start;
  end = buffer_end;
  start = str;
  range = end;
  r = onig_search(regex, str, end, start, range, region, onig_options);
  if (r != ONIG_MISMATCH) {
    count =
        region->num_regs < TS_MAX_MATCHES ? region->num_regs : TS_MAX_MATCHES;
    for (int i = 0; i < count; i++) {
      if (region->beg[i] < 0) {
        // partial match means no match
        count = 0;
        break;
      }
      if (state) {
        state->matches[i].buffer = buffer_start;
        state->matches[i].start = region->beg[i];
        state->matches[i].end = region->end[i];
      }
    }
  }

  found = count > 0;
  if (state) {
    state->count = count;
  }

  onig_end();
  onig_region_free(region, 1);

  if (found) {
    printf("matches %s\n", pattern);
  }
  return found;
}

bool tx_expand_name(char_u *scope, char_u *target, TxCaptureList capture_list) {
  char_u trailer[TS_SCOPE_NAME_LENGTH];
  char_u *trailer_advanced = NULL;
  char_u *placeholder = NULL;

  strncpy(target, scope, TS_SCOPE_NAME_LENGTH);

  bool expanded = true;

  while (placeholder = strchr(target, '$')) {
    strcpy(trailer, placeholder);
    trailer_advanced = strchr(trailer, '.');
    if (!trailer_advanced) {
      trailer_advanced = trailer;
    }
    int capture_idx = atoi(placeholder + 1);

    char_u replace[TS_SCOPE_NAME_LENGTH] = "";
    int len = capture_list[capture_idx].end - capture_list[capture_idx].start;
    if (len >= TS_SCOPE_NAME_LENGTH) {
      len = TS_SCOPE_NAME_LENGTH;
    }

    if (len != 0) {
      memcpy(replace,
             capture_list[capture_idx].buffer + capture_list[capture_idx].start,
             len * sizeof(char_u));
      strncpy(capture_list[capture_idx].name, replace, TS_SCOPE_NAME_LENGTH);
    } else {
      expanded = false;
    }

    if (replace) {
      sprintf(target + (placeholder - target), "%s%s", replace,
              trailer_advanced);
    }
  }

  return expanded;
}

static TxState match_first_pattern(char_u *buffer_start, char_u *buffer_end,
                                   TxNode *patterns, int depth);

static TxState match_first(char_u *buffer_start, char_u *buffer_end,
                           TxSyntax *syntax, int depth) {
  TxState match;
  txs_init_state(&match);

  if (syntax->rx_match && find_match(buffer_start, buffer_end, syntax->rx_match,
                                     &match, syntax->rxs_match)) {
    match.syntax = syntax;
    TxNode *n = syntax->self;
    if (n->name) {
      strcpy(match.matches[0].name, n->name);
      printf("rx_match:>>>>>>>>>>>>>>%s\n", n->name);
    }
  } else if (syntax->rx_begin &&
             find_match(buffer_start, buffer_end, syntax->rx_begin, &match,
                        syntax->rxs_begin)) {
    match.syntax = syntax;
    TxNode *n = syntax->self;
    if (n->name) {
      strcpy(match.matches[0].name, n->name);
    }
    printf("rx_begin:>>>>>>>>>>>>>>%s\n", n->name);
  } else if (syntax->rx_end) {
    // skip
  } else if (depth < TS_MAX_PATTERN_DEPTH) {
    if (syntax->captures) {
      // dump_syntax_node(synta;
      // match.syntax = syntax;
      // match.count = 1;
      // match.matches[0].start = 0;
      // match.matches[0].end = 4;
      // match.matches[0].buffer = buffer_start;
      // return match;
    }
    match =
        match_first_pattern(buffer_start, buffer_end, syntax->patterns, depth);
  }
  return match;
}

static TxState match_first_pattern(char_u *buffer_start, char_u *buffer_end,
                                   TxNode *patterns, int depth) {
  TxState earliest_match;
  txs_init_state(&earliest_match);

  if (!patterns) {
    return earliest_match;
  }

  TxNode *p = patterns->first_child;
  while (p) {
    TxState match;
    txs_init_state(&match);

    TxNode *pattern_node = p;
    TxSyntax *pattern_syntax = txn_syntax_value_proxy(p);

    match = match_first(buffer_start, buffer_end, pattern_syntax, depth + 1);
    if (match.count) {
      if (match.matches[0].start == 0) {
        earliest_match = match;
        break;
      }

      if (!earliest_match.syntax ||
          earliest_match.matches[0].start > match.matches[0].start) {
        earliest_match = match;
      }
    }

    p = p->next_sibling;
  }

  return earliest_match;
}

static TxState match_end(char_u *buffer_start, char_u *buffer_end,
                         TxSyntax *syntax, TxCaptureList capture_list) {
  TxState match;
  txs_init_state(&match);

  if (!syntax->rx_end) {
    return match;
  }

  if (find_match(buffer_start, buffer_end, syntax->rx_end, &match,
                 syntax->rxs_end)) {
    TxNode *self = syntax->self;
    TxNode *parent = self->parent;
    match.syntax = syntax;
    TxNode *n = syntax->self;
    if (n->name) {
      strcpy(match.matches[0].name, n->name);
    }
    printf("rx_end:>>>>>>>>>>>>>>%s\n", n->name);
  }

  return match;
}

static void collect_captures(char_u *buffer_start, char_u *buffer_end,
                           TxState *state, TxSyntaxNode *captures,
                           TxCaptureList target_capture,
                           TxCaptureList source_capture,
                           TxProcessor *processor) {
  TxMatchRange *matches = state->matches;

  printf("??%d\n", state->count);

  // char key[64];
  for (int j = 0; j < state->count; j++) {
    int capture_idx = j;
    TxCapture *capture = &target_capture[capture_idx];

    capture->buffer = buffer_start;
    capture->start = matches[j].start;
    capture->end = matches[j].end;
    capture->scope = NULL;
    capture->name[0] = NULL;
    capture->expanded[0] = NULL;

    // sprintf(key, "%d", capture_idx);

    TxSyntax *syntax = NULL;
    if (j == 0) {
      syntax = state->syntax;
    } else {
      TxSyntaxNode *node =
          captures ? txn_syntax_value(captures)->capture_refs[capture_idx]
                   : NULL;
      syntax = node ? txn_syntax_value_proxy(node) : NULL;
    }

    if (!syntax)
      continue;
    
    char_u *name = syntax->name;
    if (name) {
      strcpy(capture->expanded, name);
    }

    printf("%d [%s] %s\n", j, state->matches[0].name, capture->expanded);

    // int len = matches[j].end - matches[j].start;
    // char_u *start = buffer_start + matches[j].start;

    // if (name) {
    //   bool matched = true;
    //   if (syntax->rx_match) {
    //     matched = find_match(start, start + len, syntax->rx_match, NULL,
    //                          syntax->rxs_match);
    //   }
    //   if (matched) {
    //     printf("???%s\n", name);
    //     capture->scope = name;
    //     strcpy(capture->expanded, name);
    //   }
    // }
  }

  expand_captures(state, target_capture, source_capture, processor);
}

bool tx_rebuild_end_pattern(char_u *pattern, char_u *target,
                            TxCaptureList capture_list) {

  char_u new_pattern[TS_SCOPE_NAME_LENGTH] = "";
  strncpy(new_pattern, pattern, TS_SCOPE_NAME_LENGTH);
  bool dynamic = false;

#ifdef TX_SYNTAX_RECOMPILE_REGEX_END

  char_u *capture_keys[] = {"\\1", "\\2", "\\3", "\\4", "\\5",
                            "\\6", "\\7", "\\8", "\\9", 0};

  char_u trailer[TS_SCOPE_NAME_LENGTH];

  for (int j = 0; j < TS_MAX_CAPTURES; j++) {
    char_u *pos;
    if (pos = strstr(new_pattern, capture_keys[j])) {
      strcpy(trailer, pos + strlen(capture_keys[j]));
      char_u replace[TS_SCOPE_NAME_LENGTH] = "[a-zA-Z]*";

      dynamic = true;

      if (capture_list) {
        int capture_idx = j;
        int len =
            capture_list[capture_idx].end - capture_list[capture_idx].start;
        if (len >= TS_SCOPE_NAME_LENGTH) {
          len = TS_SCOPE_NAME_LENGTH;
        }
        if (len != 0) {
          memcpy(replace,
                 capture_list[capture_idx].buffer +
                     capture_list[capture_idx].start,
                 len * sizeof(char_u));
          replace[len] = 0;
        }
      }

      sprintf(pos, "%s%s", replace, trailer);
    }
  }
#endif
  strncpy(target, new_pattern, TS_SCOPE_NAME_LENGTH);
  return dynamic;
}

void tx_parse_line(char_u *buffer_start, char_u *buffer_end,
                   TxStateStack *stack, TxProcessor *processor) {
  char_u *start = buffer_start;
  char_u *end = buffer_end;

  if (processor && processor->line_start) {
    processor->line_start(processor, stack, buffer_start,
                          buffer_end - buffer_start);
  }

  while (true) {
    TxState *top = txs_top(stack);
    TxSyntax *syntax = top->syntax;

    end = buffer_end;

    TxState pattern_match;
    txs_init_state(&pattern_match);
    TxState end_match;
    txs_init_state(&end_match);

    if (syntax->patterns) {
      pattern_match = match_first_pattern(start, end, syntax->patterns, 0);
      pattern_match.offset = start - buffer_start;
    }

    if (syntax->rx_end) {
      end_match = match_end(start, end, syntax, top->matches);
      end_match.offset = start - buffer_start;
    }

    if (end_match.syntax &&
        (!pattern_match.syntax ||
         pattern_match.matches[0].start >= end_match.matches[0].start)) {
      pattern_match = end_match;

      // process captures
      if (pattern_match.syntax->captures) {
        collect_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures, pattern_match.matches,
                       txs_top(stack)->matches, NULL);
      }
      if (pattern_match.syntax->end_captures) {
        collect_captures(start, end, &pattern_match,
                       pattern_match.syntax->end_captures,
                       pattern_match.matches, pattern_match.matches, processor);
      }

      end = start + pattern_match.matches[0].end;
      start = start + pattern_match.matches[0].start;

      // if (processor && processor->close_tag) {
      //   txs_top(stack)->matches[0].end =
      //       pattern_match.offset + pattern_match.matches[0].end;
      //   processor->close_tag(processor, stack);
      // }

      txs_pop(stack);

    } else {

      // exit loop if no match has been found
      if (!pattern_match.count) {
        break;
      }

      if (pattern_match.syntax->rx_begin) {
        if (pattern_match.syntax->captures) {
          collect_captures(start, end, &pattern_match,
                         pattern_match.syntax->captures, pattern_match.matches,
                         pattern_match.matches, processor);

          // todo... syntax node must be set a load time... dynamic data should
          // all be at TxState
          // tx_rebuild_end_pattern(pattern_match.syntax,
          //                     pattern_match.syntax->end_pattern,
          //                     pattern_match.matches);
        }

        txs_push(stack, &pattern_match);
        // if (processor && processor->open_tag) {
        //   strcpy(txs_top(stack)->matches[0].expanded,
        //          pattern_match.matches[0].expanded);
        //   processor->open_tag(processor, stack);
        // }

        end = start + pattern_match.matches[0].end;
        start = start + pattern_match.matches[0].start;

      } else if (pattern_match.syntax->rx_match) {
        collect_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures, pattern_match.matches,
                       pattern_match.matches, processor);

        end = start + pattern_match.matches[0].end;
        start = start + pattern_match.matches[0].start;
      }
    }

    start = end;
  }

  if (processor && processor->line_end) {
    processor->line_end(processor, stack);
  }

  if (stack->size >= TS_MAX_STACK_DEPTH) {
    stack->size = 1;
    // oops moment -- todo.. reduce
  }
}

/**********************
 * processor
 **********************/

static void open_tag(TxProcessor *self, TxStateStack *stack) {
  TxState *top = txs_top(stack);
  TxState state;
  memcpy(&state, top, sizeof(TxState));
  state.matches[0].end = self->length;
  txs_push(&self->state_stack, &state);
}

static void close_tag(TxProcessor *self, TxStateStack *stack) {
  TxState *top = txs_top(stack);

  for (int i = self->state_stack.size; i > 0; i--) {
    TxState *state = &self->state_stack.states[i - 1];
    for (int j = state->count; j > 0; j--) {
      if (strcmp(state->matches[j - 1].expanded, top->matches[0].expanded) ==
          0) {
        state->matches[j - 1].end = top->offset + top->matches[0].end;
      }
    }
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

static void line_end(TxProcessor *self, TxStateStack *stack) {}

void txp_line_processor(TxProcessor *processor) {
  processor->line_start = line_start;
  processor->line_end = line_end;
  processor->open_tag = open_tag;
  processor->close_tag = close_tag;
}