#include "textmate.h"
#include <stdio.h>
#include <string.h>

static char_u temp[1024];
char_u* dump_range(char_u* anchor, size_t start, size_t end)
{
  char_u *buffer = anchor;
  int len = (end - start);
  memcpy(temp, buffer + start, len * sizeof(char_u));
  temp[len] = 0;
  return temp;
}

void dump_syntax(TxSyntax *syntax) {
  TxNode *node = syntax->self;
  TxNode *parent = node->parent;
  printf("> %s\n", syntax->name);
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

static bool find_match(char_u* anchor, char_u *buffer_start, char_u *buffer_end, regex_t *regex, char_u *pattern, TxState *_state) {
  bool found = false;

  TxState null_state;
  txs_init_state(&null_state);
  TxState *state = _state;
  if (!state) {
    state = &null_state;
  }
  OnigRegion *region = onig_region_new();
  int r;
  unsigned char *start, *range, *end;
  unsigned int onig_options = ONIG_OPTION_NONE;

  int offset = buffer_start - anchor;

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
      state->matches[i].buffer = anchor;
      state->matches[i].start = region->beg[i] + offset;
      state->matches[i].end = region->end[i] + offset;
    }
  }

  state->size = count;
  found = count > 0;

  onig_end();
  onig_region_free(region, 1);

  if (found) {
    printf("matches %s (%d-%d)", pattern, state->matches[0].start, state->matches[0].end);
    printf(dump_range(anchor, state->matches[0].start, state->matches[0].end));
    printf("\n");
  }
  return found;
}

static TxState match_first_pattern(char_u* anchor, char_u *start, char_u *end,
                                   TxNode *patterns);

static TxState match_first(char_u* anchor, char_u *start, char_u *end,
                           TxSyntax *syntax) {
  TxState match;
  txs_init_state(&match);

  if (syntax->rx_match &&
      find_match(anchor, start, end, syntax->rx_match, syntax->rxs_match, &match)) {
      match.syntax = syntax;
  } else if (syntax->rx_begin &&
             find_match(anchor, start, end, syntax->rx_begin, syntax->rxs_match, &match)) {
      match.syntax = syntax;
  } else if (syntax->rx_end) {
    // skip
  } else {
    match =
        match_first_pattern(anchor, start, end, syntax->patterns);
  }
  return match;
}

static TxState match_first_pattern(char_u* anchor, char_u *start, char_u *end,
                                   TxNode *patterns) {
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

    match = match_first(anchor, start, end, pattern_syntax);
    if (match.syntax) {
      // if (match.matches[0].start == 0) {
      //   earliest_match = match;
      //   break;
      // }

      if (!earliest_match.syntax ||
          earliest_match.matches[0].start > match.matches[0].start) {
        earliest_match = match;
      }
    }

    p = p->next_sibling;
  }

  return earliest_match;
}

static TxState match_end(char_u *anchor, char_u *start, char_u *end,
                         TxSyntax *syntax) {
  TxState match;
  txs_init_state(&match);

  if (!syntax->rx_end) {
    return match;
  }

  if (find_match(anchor, start, end, syntax->rx_end, syntax->rxs_end, &match)) {
    match.syntax = syntax;
  }

  return match;
}

void collect_captures(char_u* anchor, TxState *state, TxSyntaxNode *captures) {
  if (!captures) {
    for(int i=0; i<state->size; i++) {
      TxMatchRange *m = &state->matches[i];
      printf("\t>> (%d %d) ", m->start, m->end);
      printf(dump_range(anchor, m->start, m->end));
      printf("\n");
    }
    return;
  }

  // dump(captures, 0);

  char_u *capture_keys[] = {"1", "2", "3", "4", "5",
                        "6", "7", "8", "9", 0};
  for (int j = 0; j < TS_MAX_MATCHES; j++) {
    char_u *capture_key = capture_keys[j];
    if (!capture_key)
      break;

    TxMatchRange *m = &state->matches[j+1];

    TxNode* n = txn_get(captures, capture_key);
    if (n) {
      TxNode *scope = txn_get(n, "name");
      if (scope) {
        printf(":: %d %s [", j, scope->string_value);
        printf(dump_range(anchor, m->start, m->end));
        printf("]\n");
      }
    }
  }
}

void tx_parse_line(char_u *buffer_start, char_u *buffer_end, TxStateStack *stack) {
  char_u *start = buffer_start;
  char_u *end = buffer_end;
  char_u *anchor = buffer_start;

  while(true) {
    TxState *top = txs_top(stack);
    TxSyntax *syntax = top->syntax;

    TxState pattern_match;
    txs_init_state(&pattern_match);
    TxState end_match;
    txs_init_state(&end_match);

    printf("stack: %d offset: %d\n", stack->size, start - buffer_start);
    end = buffer_end;

    if (syntax->patterns) {
      pattern_match = match_first_pattern(anchor, start, end, syntax->patterns);
      // pattern_match.offset = start - buffer_start;

      if (pattern_match.syntax) {
        printf("+ %d %d ", pattern_match.matches[0].start, pattern_match.matches[0].end);
        printf(dump_range(anchor, pattern_match.matches[0].start, pattern_match.matches[0].end));
        printf(" <<<<< \n");
        dump_syntax(pattern_match.syntax);
      }
    }

    if (syntax->rx_end) {
      end_match = match_end(anchor, start, end, syntax);
      end_match.offset = start - buffer_start;
    }

    if (end_match.syntax &&
        (!pattern_match.syntax ||
         pattern_match.matches[0].start >= end_match.matches[0].start)) {
      pattern_match = end_match;

      end = buffer_start + pattern_match.matches[0].end;
      start = buffer_start + pattern_match.matches[0].start;

      txs_pop(stack);

      if (pattern_match.syntax->end_captures) {
        printf("end captures:\n");
        collect_captures(anchor, &pattern_match, pattern_match.syntax->end_captures);
      }
      dump_syntax(pattern_match.syntax);
      printf("}}}}\n");
    } else {
      if (!pattern_match.syntax) {
        break;
      }

      // pattern_match.offset = start - buffer_start;
      end = buffer_start + pattern_match.matches[0].end;
      start = buffer_start + pattern_match.matches[0].start;

      if (pattern_match.syntax->rx_begin) {
        txs_push(stack, &pattern_match);

        printf("{{{{\n");
        printf(dump_range(anchor, pattern_match.matches[0].start, pattern_match.matches[0].end));
        printf("\n");
        dump_syntax(pattern_match.syntax);

        if (pattern_match.syntax->begin_captures) {
          printf("begin captures:\n");
          collect_captures(anchor, &pattern_match, pattern_match.syntax->begin_captures);
        }

      } else if (pattern_match.syntax->rx_match) {
        // process captures

        if (pattern_match.syntax->captures) {
          printf("captures:\n");
          collect_captures(anchor, &pattern_match, pattern_match.syntax->captures);
        }

      } else {
      }
    }

    start = end;
  }
}
