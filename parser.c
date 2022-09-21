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
        state->matches[i].buffer = buffer_start;
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

static void expand_name(char_u *scope, char_u *target, TxCaptureList capture_list) {
    char_u trailer[TS_SCOPE_NAME_LENGTH];
    char_u *trailer_advanced = NULL;
    char_u *placeholder = NULL;

    strncpy(target, scope, TS_SCOPE_NAME_LENGTH);

    while (placeholder = strchr(target, '$')) {
      strcpy(trailer, placeholder);
      trailer_advanced = strchr(trailer, '.');
      if (!trailer_advanced) {
        trailer_advanced = trailer;
      }
      int capture_idx = atoi(placeholder + 1);

      char_u replace[TS_CAPTURE_NAME_LENGTH] = "???";
      int len = capture_list[capture_idx - 1].end - capture_list[capture_idx - 1].start;
      if (len >= TS_CAPTURE_NAME_LENGTH) {
        len = TS_CAPTURE_NAME_LENGTH;
      }
      if (len != 0) {
        memcpy(replace,
               capture_list[capture_idx - 1].buffer +
                   capture_list[capture_idx - 1].start,
               len * sizeof(char_u));
        strncpy(capture_list[capture_idx - 1].name, replace, TS_CAPTURE_NAME_LENGTH);
      }

      sprintf(target + (placeholder - target), "%s%s",
              replace, trailer_advanced);
    }
}

static void dump_match(TxState *state) {
  char_u* name = state->syntax->scope_name;
  if (!name) {
    name = state->syntax->content_name;
  }
  if (!name) {
    name = state->syntax->name;
  }
  if (name) {
    char_u expanded[TS_SCOPE_NAME_LENGTH] = "";
    expand_name(name, expanded, state->matches);
    printf("<%ld-%ld> [%s]\n", state->matches[0].start,
           state->matches[0].end, expanded);
  }
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

      TxNode *include_node = txn_get(pattern_syntax, "include");
      if (include_node && include_node->string_value) {
        char_u *path = include_node->string_value;

        // todo request external syntax
        // resolve source scope
        // resolve #hash
        // tx_syntax_from_scope
        // resolve #hash from local repository
      }

      printf("request external syntax\n");
    }

    if (pattern_syntax->include) {
      pattern_node = pattern_syntax->include;
      pattern_syntax = txn_syntax_value(pattern_node);

      if (!pattern_syntax->rx_match && !pattern_syntax->rx_begin) {
        TxNode *more_patterns = txn_get(pattern_node, "patterns");
        // depth is for circular reference protection
        if (more_patterns && depth == 0) {
          match = match_patterns(buffer_start, buffer_end, more_patterns, depth + 1);
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

#if 1
  if (earliest_match.count) {
    // TxNode *name = txn_get(earliest_match.syntax->self, "name");
    dump_match(&earliest_match);
  }
  // printf("%d\n", earliest_match.count);
#endif

  return earliest_match;
}

static TxState match_end(char_u *buffer_start, char_u *buffer_end, TxSyntax *syntax) {

#if 0
    if (syntax->dynamic_end) {
      if (syntax->rx_end) {
        onig_free(syntax->rx_end);
        syntax->rx_end = NULL;
      }

      // todo handle "(?i)(</)(\\2)\\s*(>)"
      char_u *capture_keys[] = {"\\1",  "\\2",  "\\3",  "\\4",  "\\5",
                    "\\6",  "\\7",  "\\8",  "\\9",  "\\10", "\\11",
                    "\\12", "\13", "\\14", "\\15", "\\16", 0};

      int len = strlen(syntax->end_pattern) + 128;
      char_u *new_pattern = tx_malloc(len);
      char_u *trailer = tx_malloc(len);
      strcpy(new_pattern, syntax->end_pattern);
      for(int j=0; j<TS_MAX_CAPTURES; j++) {
        char_u *pos;
        if (pos = strstr(new_pattern, capture_keys[j])) {
          strcpy(trailer, pos);
          sprintf(pos, "??%s", trailer);
        }
      }

      printf(">>>%s\n", new_pattern);

      tx_free(new_pattern);
      tx_free(trailer);
    }
#endif

  TxState match;
  txs_init_state(&match);

  if (find_match(buffer_start, buffer_end, syntax->rx_end, &match)) {
    match.syntax = syntax;
  }

  return match;
}

static void match_captures(char_u *buffer_start, char_u *buffer_end,
                           TxState *state, TxSyntaxNode *captures,
                           TxCaptureList capture_list,
                           TxProcessor *processor) {
  if (!captures) {
    return;
  }

  TxMatchRange *matches = state->matches;
  memset(capture_list, 0, sizeof(TxCaptureList));

  char key[64];
  for (int j = 1; j < state->count; j++) {
    int capture_idx = j;
    TxCapture *capture = &capture_list[capture_idx-1];

    capture->buffer = buffer_start;
    capture->start = matches[j].start;
    capture->end = matches[j].end;
    capture->scope = NULL;
    capture->name[0] = NULL;
    capture->expanded[0] = NULL;

    sprintf(key, "%d", capture_idx);

    TxSyntaxNode *node = txn_get(captures, key);
    if (!node) {
      continue;
    }

    TxSyntax *syntax = txn_syntax_value(node);
    char_u* name = syntax->name;

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

  for (int j = 1; j < state->count; j++) {
    int capture_idx = j;
    if (capture_idx >= TS_MAX_CAPTURES)
      break;
    TxCapture *capture = &capture_list[capture_idx - 1];

    if (!capture->scope) {
      continue;
    }
  }

  for (int j = 1; j < state->count; j++) {
    int capture_idx = j;
    if (capture_idx >= TS_MAX_CAPTURES)
      break;
    TxCapture *capture = &capture_list[capture_idx - 1];

    if (!capture->scope) {
      continue;
    }

    expand_name(capture->scope, capture->expanded, capture_list);
  }

  // expand scopes
  #if 0
  for (int j = 1; j < state->count; j++) {
    int capture_idx = j;
    if (capture_idx >= TS_MAX_CAPTURES)
      break;
    TxCapture *capture = &capture_list[capture_idx - 1];

    if (!capture->scope) {
      continue;
    }

    char_u trailer[TS_SCOPE_NAME_LENGTH];
    char_u *trailer_advanced = NULL;
    char_u *placeholder = NULL;
    while (placeholder = strchr(capture->expanded, '$')) {
      strcpy(trailer, placeholder);
      trailer_advanced = strchr(trailer, '.');
      if (!trailer_advanced) {
        trailer_advanced = trailer;
      }
      int capture_idx = atoi(placeholder + 1);

      char_u replace[TS_CAPTURE_NAME_LENGTH] = "???";
      int len = capture_list[capture_idx - 1].end - capture_list[capture_idx - 1].start;
      if (len >= TS_CAPTURE_NAME_LENGTH) {
        len = TS_CAPTURE_NAME_LENGTH;
      }
      if (len != 0) {
        memcpy(replace,
               capture_list[capture_idx - 1].buffer +
                   capture_list[capture_idx - 1].start,
               len * sizeof(char_u));
        strncpy(capture->name, replace, TS_CAPTURE_NAME_LENGTH);
      }

      sprintf(capture->expanded + (placeholder - capture->expanded), "%s%s",
              replace, trailer_advanced);
    }

    // printf("(%ld-%ld) [%s]\n", capture->start, capture->end,
    //   capture->expanded);
  }
  #endif

  for (int i = 0;; i++) {
    TxCapture *capture = &capture_list[i];
    if (!capture->buffer)
      break;
    if (!capture->scope)
      continue;

    printf("(%ld-%ld) [%s]\n", capture->start, capture->end, capture->expanded);
  }
}

void tx_parse_line(char_u *buffer_start, char_u *buffer_end,
                   TxStateStack *stack, TxProcessor *processor) {
  char_u *start = buffer_start;
  char_u *end = buffer_end;

  if (processor && processor->line_begin) {
    processor->line_begin(processor, stack);
  }

  char_u *prev_start;
  TxSyntax *prev_match_syntax;
  while (true) {
    TxState *top = txs_top(stack);
    TxSyntax *syntax = top->syntax;

    // todo .. current sytnax dictates region to scan (not the entire line)
    end = buffer_end;

    TxState pattern_match;
    txs_init_state(&pattern_match);
    TxState end_match;
    txs_init_state(&end_match);

    if (syntax->patterns) {
      pattern_match = match_patterns(start, end, syntax->patterns, 0);
    }

    if (syntax->rx_end) {
      end_match = match_end(start, end, syntax);
    }

    if (end_match.count &&
        (!pattern_match.count ||
         pattern_match.matches[0].start <= end_match.matches[0].start)) {
      pattern_match = end_match;

      // process captures
      if (pattern_match.syntax->captures) {
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures,
                       pattern_match.capture_list,
                       processor);
      }
      if (pattern_match.syntax->end_captures) {
        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->end_captures,
                       pattern_match.end_capture_list,
                       processor);
      }

      end = start + pattern_match.matches[0].end;
      start = start + pattern_match.matches[0].start;
      txs_pop(stack);

      // processor
      printf("}}}}}}}}}}\n");
    } else {

      // exit loop if no match has been found
      if (!pattern_match.count) {
        break;
      }

      if (pattern_match.syntax->rx_begin) {
        if (pattern_match.syntax == syntax && pattern_match.matches[0].start == 0) {
          // guard for possible endless loop?
          break;
        }

        if (pattern_match.syntax->captures) {
          match_captures(start, end, &pattern_match,
                         pattern_match.syntax->captures,
                         pattern_match.capture_list,
                         processor);
        }

        end = start + pattern_match.matches[0].end;
        start = start + pattern_match.matches[0].start;
        txs_push(stack, &pattern_match);

        // processor
        printf("{{{{{{{{{{\n");
      } else if (pattern_match.syntax->rx_match) {

        match_captures(start, end, &pattern_match,
                       pattern_match.syntax->captures,
                       pattern_match.capture_list,
                       processor);

        printf("[[[[[[[[\n");
        dump_match(&pattern_match);
        printf("]]]]]]]]\n");
      }
    }

    start = end;
  }

  // printf("stack size: %d\n", stack->size);
}