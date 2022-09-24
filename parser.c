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
                          TxCaptureList source_capture, TxProcessor *processor) {
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

  if (processor && processor->capture) {
    processor->capture(processor, state, target_capture);
  }
  // expand_match(state);
}

TxSyntax *txn_syntax_value_proxy(TxSyntaxNode *node) {
  TxSyntax *syntax = txn_syntax_value(node);

  if (syntax) {
    if (syntax->include_scope) {
      printf("request external syntax %s\n", syntax->include_scope);
      TxSyntaxNode* include_node = tx_syntax_from_scope(syntax->include_scope);
      syntax->include_scope = NULL;
      if (include_node) {
        syntax->include = include_node;
        syntax = txn_syntax_value_proxy(include_node);

        // todo request external syntax
        // resolve source scope
        // resolve #hash
        // tx_syntax_from_scope
        // resolve #hash from local repository
      }
    }

    if (syntax->include) {
      TxSyntaxNode *include_node = syntax->include;
      syntax = txn_syntax_value_proxy(include_node);
    }
  }

  return syntax;
}

static bool find_match(char_u *buffer_start, char_u *buffer_end, regex_t *regex,
                       TxState *state) {
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

  if (syntax->rx_match &&
      find_match(buffer_start, buffer_end, syntax->rx_match, &match)) {
    match.syntax = syntax;
  } else if (syntax->rx_begin &&
             find_match(buffer_start, buffer_end, syntax->rx_begin, &match)) {
    match.syntax = syntax;
  } else if (syntax->rx_end) {
    // skip
  } else if (depth < TS_MAX_PATTERN_DEPTH) {
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

  if (find_match(buffer_start, buffer_end, syntax->rx_end, &match)) {
    TxNode *self = syntax->self;
    TxNode *parent = self->parent;
    match.syntax = syntax;
  }

  return match;
}

static void match_captures(char_u *buffer_start, char_u *buffer_end,
                           TxState *state, TxSyntaxNode *captures,
                           TxCaptureList target_capture,
                           TxCaptureList source_capture,
                           TxProcessor *processor) {
  TxMatchRange *matches = state->matches;

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

    int len = matches[j].end - matches[j].start;
    char_u *start = buffer_start + matches[j].start;

    if (name) {
      bool matched = true;
      if (syntax->rx_match) {
        matched = find_match(start, start + len, syntax->rx_match, NULL);
      }
      if (matched) {
        capture->scope = name;
        strcpy(capture->expanded, name);
      }
    }
  }

  expand_captures(state, target_capture, source_capture, processor);
}

static void rebuild_end_pattern(TxSyntax *syntax, char_u *pattern,
                                TxCaptureList capture_list) {

#ifdef TX_SYNTAX_RECOMPILE_REGEX_END
  if (syntax->end_pattern) {
    char_u *capture_keys[] = {"\\1", "\\2", "\\3", "\\4", "\\5",
                              "\\6", "\\7", "\\8", "\\9", 0};

    int len = strlen(syntax->end_pattern) + 128;
    char_u new_pattern[TS_SCOPE_NAME_LENGTH];
    char_u trailer[TS_SCOPE_NAME_LENGTH];
    strcpy(new_pattern, syntax->end_pattern);

    for (int j = 0; j < TS_MAX_CAPTURES; j++) {
      char_u *pos;
      if (pos = strstr(new_pattern, capture_keys[j])) {
        strcpy(trailer, pos + strlen(capture_keys[j]));
        char_u replace[TS_SCOPE_NAME_LENGTH] = "[a-zA-Z]*";

        int capture_idx = j + 1;
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

        sprintf(pos, "%s%s", replace, trailer);
      }
    }

    regex_t *new_regex = tx_compile_pattern(new_pattern);
    if (new_regex) {
      // printf("recompile regex %s\n", new_pattern);
      if (syntax->rx_end) {
        onig_free(syntax->rx_end);
        syntax->rx_end = NULL;
      }
      syntax->rx_end = new_regex;
    }
  }

#endif
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

#if 0
    TxNode *self = syntax->self;
    TxNode *parent = self->parent;
    TxNode *grand_parent = parent ? parent->parent : NULL;
    // printf(">> %d\n", start - buffer_start);
    printf("# %d %x %s.%s.%s %d/%d #\n", stack->size, syntax,
           grand_parent ? grand_parent->name : NULL,
           parent ? parent->name : NULL, self->name, self->index, parent->size);
#endif

    // todo .. current sytnax dictates region to scan (not the entire line)
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
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures, pattern_match.matches,
                       txs_top(stack)->matches, NULL);
      }
      if (pattern_match.syntax->end_captures) {
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->end_captures,
                       pattern_match.matches, pattern_match.matches, processor);
      }

      end = start + pattern_match.matches[0].end;
      start = start + pattern_match.matches[0].start;

      if (processor && processor->close_tag) {
        txs_top(stack)->matches[0].end = pattern_match.offset + pattern_match.matches[0].end;
        processor->close_tag(processor, stack);
      }

      txs_pop(stack);

    } else {

      // exit loop if no match has been found
      if (!pattern_match.count) {
        break;
      }

      if (pattern_match.syntax->rx_begin) {
        if (pattern_match.syntax->captures) {
          match_captures(start, end, &pattern_match,
                         pattern_match.syntax->captures, pattern_match.matches,
                         pattern_match.matches, processor);

          // todo... syntax node must be set a load time... dynamic data should
          // all be at TxState
          rebuild_end_pattern(pattern_match.syntax,
                              pattern_match.syntax->end_pattern,
                              pattern_match.matches);
        }

        txs_push(stack, &pattern_match);
        if (processor && processor->open_tag) {
          strcpy(txs_top(stack)->matches[0].expanded, pattern_match.matches[0].expanded);
          processor->open_tag(processor, stack);
        }

        end = start + pattern_match.matches[0].end;
        start = start + pattern_match.matches[0].start;

      } else if (pattern_match.syntax->rx_match) {
        match_captures(start, end, &pattern_match,
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