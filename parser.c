#include "textmate.h"
#include <stdio.h>
#include <string.h>

static bool find_match(char_u *buffer_start, char_u *buffer_end, regex_t *regex,
                       TxState *state) {
  bool found = false;

  OnigRegion *region = onig_region_new();
  int r;
  unsigned char *start, *range, *end;
  unsigned int onig_options = ONIG_OPTION_NONE;

  char temp[128] = "";
  UChar *str = (UChar *)buffer_start;
  end = buffer_end;
  start = str;
  range = end;
  r = onig_search(regex, str, end, start, range, region, onig_options);
  if (r != ONIG_MISMATCH) {
    int count =
        region->num_regs < TS_MAX_MATCHES ? region->num_regs : TS_MAX_MATCHES;
    if (state) {
      state->count = count;
      for (int i = 0; i < state->count; i++) {
        state->matches[i].start = region->beg[i];
        state->matches[i].end = region->end[i];
      }
    }
    found = true;
  }

  onig_end();
  onig_region_free(region, 1);
  return found;
}

static TxState match_patterns(char_u *buffer_start, char_u *buffer_end,
                              TxNode *patterns, int depth) {
  TxState earliest_match;
  txs_init_state(&earliest_match);

  TxNode *p = patterns->first_child;
  while (p) {
    TxState match;
    txs_init_state(&match);

    TxNode *pattern_node = p;
    TxSyntax *pattern_syntax = txn_syntax_value(p);

    if (!pattern_syntax->include && pattern_syntax->include_external) {
      // todo request external syntax
      printf("request external syntax\n");
    }

    if (pattern_syntax->include) {
      pattern_node = pattern_syntax->include;
      pattern_syntax = txn_syntax_value(pattern_node);

      if (!pattern_syntax->rx_match && !pattern_syntax->rx_begin) {
        TxNode *more_patterns = txn_get(pattern_node, "patterns");
        // depth is for circular reference protection
        if (more_patterns && depth == 0) {
          match = match_patterns(buffer_start, buffer_end, more_patterns, 1);
        }
      }
    }

    regex_t *regex = pattern_syntax->rx_begin ? pattern_syntax->rx_begin
                                              : pattern_syntax->rx_match;
    if (regex) {
      if (find_match(buffer_start, buffer_end, regex, &match)) {
        match.syntax = pattern_syntax;
      }
    }

    if (match.count) {
      if (!earliest_match.syntax ||
          earliest_match.matches[0].start > match.matches[0].start) {
        earliest_match = match;
        if (earliest_match.matches[0].start == 0)
          break;
      }
    }

    p = p->next_sibling;
  }

  if (earliest_match.count) {
    TxNode *name = txn_get(earliest_match.syntax->self, "name");
    if (name) {
      printf("[%s]\n", name ? name->string_value : "");
    }
  }

  // printf("%d\n", earliest_match.count);
  return earliest_match;
}

static void match_captures(char_u *buffer_start, char_u *buffer_end,
                           TxState *state, TxSyntaxNode *captures) {

  if (!captures) {
    return;
  }

  TxMatchRange *matches = state->matches;

  char key[64];
  for (int j = 1; j < state->count; j++) {
    sprintf(key, "%d", (j - 1));

    TxSyntaxNode *node = txn_get(captures, key);
    if (!node)
      continue;

    // todo does captures test for #include?

    TxSyntax *syntax = txn_syntax_value(node);
    TxSyntaxNode *name = txn_get(node, "name");

    int len = matches[j].end - matches[j].start;
    char_u *start = buffer_start + matches[j].start;

    if (name) {
      bool matched = true;
      if (syntax->rx_match) {
        matched = find_match(start, start + len, syntax->rx_match, NULL);
      }
      if (matched) {
        printf("------ [%s]\n", name->string_value);
      }
    }

    // todo processor

    // char tmp[128] = "";
    // if (len == 0)
    //   continue;
    // memcpy(tmp, buffer_start + matches[j].start, len * sizeof(char_u));
    // tmp[len] = 0;
    // printf("(%ld-%ld) %s\n", matches[j].start, matches[j].end, tmp);
  }
}

void tx_parse_line(char_u *buffer_start, char_u *buffer_end,
                   TxStateStack *stack) {
  char_u *start = buffer_start;
  char_u *end = buffer_end;

  while (true) {
    TxState *top = txs_top(stack);
    TxSyntax *syntax = top->syntax;

    // todo
    end = buffer_end;

    TxState pattern_match;
    txs_init_state(&pattern_match);
    TxState end_match;
    txs_init_state(&end_match);

    if (syntax->patterns) {
      pattern_match = match_patterns(start, end, syntax->patterns, 0);
      // printf("process patterns %d %d\n", syntax->patterns->size,
      // pattern_match.count);
    }

    if (syntax->rx_end) {
      if (find_match(start, end, syntax->rx_end, &end_match)) {
        end_match.syntax = syntax;
      }
    }

    if (end_match.count &&
        (!pattern_match.count ||
         pattern_match.matches[0].start <= end_match.matches[0].start)) {
      pattern_match = end_match;

      // process captures
      if (pattern_match.syntax->captures) {
        // printf("captures! %ld %d\n", pattern_match.syntax->captures->size,
        // pattern_match.count);
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures);
      }
      if (pattern_match.syntax->end_captures) {
        // printf("end captures! %ld %d\n",
        // pattern_match.syntax->captures->size, pattern_match.count);
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->end_captures);
      }

      end = start + pattern_match.matches[0].end;
      start = start + pattern_match.matches[0].start;
      txs_pop(stack);
    } else {

      // exit loop if no match has been found
      if (!pattern_match.count) {
        // printf("no match\n");
        break;
      }

      if (pattern_match.syntax->rx_begin) {
        pattern_match.offset = start - buffer_start;

        if (pattern_match.syntax->captures) {
          // printf("begin captures! %ld %d\n",
          // pattern_match.syntax->captures->size, pattern_match.count);
          match_captures(start, end, &pattern_match,
                         pattern_match.syntax->captures);
        }

        end = start + pattern_match.matches[0].end;
        start = start + pattern_match.matches[0].start;
        txs_push(stack, &pattern_match);
      } else if (pattern_match.syntax->rx_match) {
        // printf("match captures! %ld %d\n",
        // pattern_match.syntax->captures->size, pattern_match.count);
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures);
      }
    }

    start = end;
  }
}