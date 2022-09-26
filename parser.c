#include "textmate.h"
#include <stdio.h>
#include <string.h>

#define _BEGIN_COLOR(R, G, B) printf("\x1b[38;2;%d;%d;%dm", R, G, B);
#define _END_COLOR printf("\x1b[0m");

// #define _DEBUG_CAPTURES

static char_u *extract_buffer_range(char_u *anchor, size_t start, size_t end) {
  static char_u temp[TX_SCOPE_NAME_LENGTH];
  char_u *buffer = anchor;
  int len = (end - start);
  if (len >= TX_SCOPE_NAME_LENGTH) {
    len = TX_SCOPE_NAME_LENGTH - 1;
  }
  memcpy(temp, buffer + start, len * sizeof(char_u));
  temp[len] = 0;
  return temp;
}

static bool expand_name(char_u *scope, char_u *target, TxMatch *state) {
  if (!scope) {
    strcpy(target, "");
    return false;
  }
  char_u trailer[TX_SCOPE_NAME_LENGTH];
  char_u *trailer_advanced = NULL;
  char_u *placeholder = NULL;

  TxMatchRange *capture_list = state->matches;

  strncpy(target, scope, TX_SCOPE_NAME_LENGTH);

  bool expanded = true;

  while (placeholder = strchr(target, '$')) {
    strcpy(trailer, placeholder);
    trailer_advanced = strchr(trailer, '.');
    if (!trailer_advanced) {
      trailer_advanced = trailer;
    }
    int capture_idx = atoi(placeholder + 1);

    char_u replace[TX_SCOPE_NAME_LENGTH] = "";
    int len = capture_list[capture_idx].end - capture_list[capture_idx].start;
    if (len >= TX_SCOPE_NAME_LENGTH) {
      len = TX_SCOPE_NAME_LENGTH;
    }

    if (len != 0) {
      memcpy(replace,
             capture_list[capture_idx].buffer + capture_list[capture_idx].start,
             len * sizeof(char_u));
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

static bool find_match(char_u *anchor, char_u *buffer_start, char_u *buffer_end,
                       regex_t *regex, char_u *pattern, TxMatch *_state) {
  bool found = false;

  TxMatch null_state;
  tx_init_match(&null_state);
  TxMatch *state = _state;
  if (!state) {
    state = &null_state;
  }
  OnigRegion *region = onig_region_new();
  int r;
  unsigned char *start, *range, *end;
  unsigned int onig_options = ONIG_OPTION_NONE;

  int offset = buffer_start - anchor;
  int count = 0;

  UChar *str = anchor;
  end = buffer_end;
  start = buffer_start;
  range = end;
  offset = 0;

  // UChar *str = (UChar *)buffer_start;
  // end = buffer_end;
  // start = str;
  // range = end;

  r = onig_search(regex, str, end, start, range, region, onig_options);
  if (r != ONIG_MISMATCH) {
    count =
        region->num_regs < TX_MAX_MATCHES ? region->num_regs : TX_MAX_MATCHES;
    for (int i = 0; i < count; i++) {
      if (region->beg[i] < 0) {
        // partial match means no match
        count = 0;
        break;
      }
      state->matches[i].buffer = anchor;
      state->matches[i].start = region->beg[i] + offset;
      state->matches[i].end = region->end[i] + offset;
    }
  }

  state->size = count;
  found = count > 0;

  onig_end();
  onig_region_free(region, 1);

  // if (found) {
  //   printf("matches %s (%d-%d) ", pattern, state->matches[0].start,
  //          state->matches[0].end);
  //   _BEGIN_COLOR(255, 0, 255)
  //   printf(extract_buffer_range(anchor, state->matches[0].start,
  //                               state->matches[0].end));
  //   printf("\n");
  //   _END_COLOR
  // }

  return found;
}

static TxMatch match_first_pattern(char_u *anchor, char_u *start, char_u *end,
                                   TxNode *patterns);

static TxMatch match_first(char_u *anchor, char_u *start, char_u *end,
                           TxSyntax *syntax) {
  TxMatch match;
  tx_init_match(&match);

  if (syntax->rx_match && find_match(anchor, start, end, syntax->rx_match,
                                     syntax->rxs_match, &match)) {
    match.syntax = syntax;
  } else if (syntax->rx_begin &&
             find_match(anchor, start, end, syntax->rx_begin, syntax->rxs_match,
                        &match)) {
    match.syntax = syntax;
  } else if (syntax->rx_end) {
    // skip
  } else {
    match = match_first_pattern(anchor, start, end, syntax->patterns);
  }
  return match;
}

static TxMatch match_first_pattern(char_u *anchor, char_u *start, char_u *end,
                                   TxNode *patterns) {
  TxMatch earliest_match;
  tx_init_match(&earliest_match);

  if (!patterns) {
    return earliest_match;
  }

  TxNode *p = patterns->first_child;
  while (p) {
    TxMatch match;
    tx_init_match(&match);

    TxNode *pattern_node = p;
    TxSyntax *pattern_syntax = txn_syntax_value_proxy(p);

    match = match_first(anchor, start, end, pattern_syntax);
    if (match.syntax) {
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

static TxMatch match_end(char_u *anchor, char_u *start, char_u *end,
                         TxSyntax *syntax) {
  TxMatch match;
  tx_init_match(&match);

  if (!syntax->rx_end) {
    return match;
  }

  if (find_match(anchor, start, end, syntax->rx_end, syntax->rxs_end, &match)) {
    match.syntax = syntax;
  }

  return match;
}

static void collect_match(TxSyntax *syntax, TxMatch *state,
                          TxParseProcessor *processor) {
  TxNode *node = syntax->self;
  TxNode *parent = node->parent;

  expand_name(syntax->name, state->matches[0].scope, state);
  if (processor && processor->capture) {
    processor->capture(processor, state);
  }

#ifdef _DEBUG_CAPTURES
  char_u *temp = state->matches[0].scope;
  printf("> %s\n", temp);
  printf("+ (%d-%d) ", state->matches[0].start, state->matches[0].end);
  _BEGIN_COLOR(255, 255, 0)
  printf(extract_buffer_range(state->matches[0].buffer, state->matches[0].start,
                              state->matches[0].end));
  _END_COLOR
  printf(" <<<<< \n");
#endif
}

static void collect_captures(char_u *anchor, TxMatch *state,
                             TxSyntaxNode *captures,
                             TxParseProcessor *processor) {
  char_u temp[TX_SCOPE_NAME_LENGTH];

  char_u *capture_keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", 0};
  for (int j = 0; j < TX_MAX_MATCHES; j++) {
    char_u *capture_key = capture_keys[j];
    if (!capture_key)
      break;

    TxMatchRange *m = &state->matches[j + 1];

    TxNode *n = txn_get(captures, capture_key);
    if (n) {
      TxNode *scope = txn_get(n, "name");
      if (scope) {
        expand_name(scope->string_value, temp, state);
        strncpy(m->scope, extract_buffer_range(anchor, m->start, m->end),
                TX_SCOPE_NAME_LENGTH);
#ifdef _DEBUG_CAPTURES
        printf(":: (%d-%d) %s [", m->start, m->end, temp);
        _BEGIN_COLOR(0, 255, 255)
        printf("%s", m->scope);
        _END_COLOR
        printf("]\n");
#endif
      }
    }
  }

  if (processor && processor->capture) {
    processor->capture(processor, state);
  }
}

void tx_parse_line(char_u *buffer_start, char_u *buffer_end,
                   TxParserState *stack, TxParseProcessor *processor) {
  char_u *start = buffer_start;
  char_u *end = buffer_end;
  char_u *anchor = buffer_start;

  if (processor && processor->line_start) {
    processor->line_start(processor, buffer_start, buffer_end);
  }

  while (true) {
    TxMatch *top = tx_state_top(stack);
    TxSyntax *syntax = top->syntax;

    TxMatch pattern_match;
    tx_init_match(&pattern_match);
    TxMatch end_match;
    tx_init_match(&end_match);

#ifdef _DEBUG_CAPTURES
    printf("stack: %d offset: %d\n", stack->size, start - buffer_start);
#endif

    end = buffer_end;

    if (syntax->patterns) {
      pattern_match = match_first_pattern(anchor, start, end, syntax->patterns);
    }

    if (syntax->rx_end) {
      end_match = match_end(anchor, start, end, syntax);
    }

    if (end_match.syntax &&
        (!pattern_match.syntax ||
         pattern_match.matches[0].start >= end_match.matches[0].start)) {
      pattern_match = end_match;

      end = buffer_start + pattern_match.matches[0].end;
      start = buffer_start + pattern_match.matches[0].start;

      tx_state_pop(stack);

      collect_match(pattern_match.syntax, &pattern_match, processor);

      if (pattern_match.syntax->end_captures) {
        collect_captures(anchor, &pattern_match,
                         pattern_match.syntax->end_captures, processor);
      }

#ifdef _DEBUG_CAPTURES
      printf("}}}}\n");
#endif

    } else {
      if (!pattern_match.syntax) {
        break;
      }

      collect_match(pattern_match.syntax, &pattern_match, processor);

      end = buffer_start + pattern_match.matches[0].end;
      start = buffer_start + pattern_match.matches[0].start;

      if (pattern_match.syntax->rx_begin) {

        tx_state_push(stack, &pattern_match);

#ifdef _DEBUG_CAPTURES
        printf("{{{{\n");
        printf(extract_buffer_range(anchor, pattern_match.matches[0].start,
                                    pattern_match.matches[0].end));
        printf("\n");
#endif
        if (pattern_match.syntax->begin_captures) {

          collect_captures(anchor, &pattern_match,
                           pattern_match.syntax->begin_captures, processor);
        }

      } else if (pattern_match.syntax->rx_match) {

        if (pattern_match.syntax->captures) {

          collect_captures(anchor, &pattern_match,
                           pattern_match.syntax->captures, processor);
        }
      }
    }

    start = end;
  }

  if (processor && processor->line_end) {
    processor->line_end(processor);
  }
}
