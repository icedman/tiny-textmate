#include "textmate.h"
#include <stdio.h>
#include <string.h>

// #define _DEBUG_COLLECT
// #define _DEBUG_MATCHES

extern uint32_t _match_execs;

void dump(TxNode *n, int level);

char_u *tx_extract_buffer_range(char_u *anchor, size_t start, size_t end) {
  static char_u temp[TX_SCOPE_NAME_LENGTH];
  strcpy(temp, "");
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
    strncpy(target, "", TX_SCOPE_NAME_LENGTH);
    return false;
  }
  char_u trailer[TX_SCOPE_NAME_LENGTH];
  char_u *trailer_advanced = NULL;
  char_u *placeholder = NULL;

  TxMatchRange *capture_list = state->matches;

  strncpy(target, scope, TX_SCOPE_NAME_LENGTH);

  bool expanded = true;

  while (placeholder = strchr(target, '$')) {
    strncpy(trailer, placeholder, TX_SCOPE_NAME_LENGTH);
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

      if (syntax->include_scope) {
        // printf(">me:%x\n", node);
        // printf("? include %s\n", syntax->include_scope);

        char_u ns[TX_SCOPE_NAME_LENGTH] = "";
        char_u scope[TX_SCOPE_NAME_LENGTH] = "";
        strncpy(ns, syntax->include_scope, TX_SCOPE_NAME_LENGTH);
        char_u *u = strchr(syntax->include_scope, '#');
        if (u) {
          ns[u - syntax->include_scope] = 0;
          strncpy(scope, u, TX_SCOPE_NAME_LENGTH);
        }

        TxSyntaxNode *target_node = NULL;

        if (strlen(ns) > 0 && ns[0] != '#') {
          target_node = tx_syntax_from_scope(ns);
          #ifdef _DEBUG_COLLECT
          _BEGIN_UNDERLINE
          _BEGIN_COLOR(255,0,0)
          printf("global %s %x\n", ns, txn_syntax_value_proxy(target_node));
          _END_FORMAT
          #endif
        } else {
          strncpy(scope, syntax->include_scope, TX_SCOPE_NAME_LENGTH);
        }

        if (strlen(scope) > 0) {
          TxNode *root =
              txn_syntax_value(target_node ? target_node : node)->root;
          TxNode *repo_node = txn_get(root, "repository");
          target_node = repo_node ? txn_get(repo_node, scope + 1) : NULL;
          
          #ifdef _DEBUG_COLLECT
          printf("# include [%s] source:%x root:%x repo:%x\n", scope+1,
            target_node, root, repo_node);
          #endif
        }

        syntax->include = target_node;
      }
    }

    if (syntax->include) {
      TxSyntaxNode *include_node = syntax->include;
      syntax = txn_syntax_value_proxy(include_node);
      // printf("include: %x\n", syntax);
    }
  }

  return syntax;
}

static bool find_match(char_u *anchor, char_u *buffer_start, char_u *buffer_end,
                       regex_t *regex, char_u *pattern, TxMatch *_state) {
  bool found = false;

  _match_execs++;

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

  // int offset = buffer_start - anchor;
  int count = 0;

  UChar *str = anchor;
  end = buffer_end;
  start = buffer_start;
  range = end;
  // offset = 0;

  r = onig_search(regex, str, end, start, range, region, onig_options);
  if (r != ONIG_MISMATCH) {
    count =
        region->num_regs < TX_MAX_MATCHES ? region->num_regs : TX_MAX_MATCHES;
    for (int i = 0; i < count; i++) {
      if (region->beg[i] >= 0) {
        state->rank++;
      }
      state->matches[i].buffer = anchor;
      state->matches[i].start = region->beg[i]; // + offset;
      state->matches[i].end = region->end[i];   // + offset;
    }
  }

  state->size = count;
  found = count > 0;

  onig_end();
  onig_region_free(region, 1);

#ifdef _DEBUG_MATCHES
  if (found) {
    printf("matches %s (%d-%d) ", pattern, state->matches[0].start,
           state->matches[0].end);
    _BEGIN_COLOR(255, 0, 255)
    printf(tx_extract_buffer_range(anchor, state->matches[0].start,
                                   state->matches[0].end));
    printf("\n");
    _END_FORMAT
  }
#endif

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
             find_match(anchor, start, end, syntax->rx_begin, syntax->rxs_begin,
                        &match)) {
    match.syntax = syntax;
    match.rank++; // begin is preferred over match?
  } else if (syntax->rx_end || syntax->rx_end_dynamic || syntax->rx_while) {
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
      if (anchor + match.matches[0].start == start) {
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

  regex_t *regex = syntax->rx_end;

  // if (syntax->rx_end_dynamic && !syntax->rx_end) {
  //   printf(">%s\n", syntax->rxs_end);
  //   printf("dynamic end - not yet implemented\n");
  // }

  if (!regex) {
    return match;
  }

  if (find_match(anchor, start, end, regex, syntax->rxs_end, &match)) {
    match.syntax = syntax;
  }

  return match;
}

static void collect_match(TxSyntax *syntax, TxMatch *state,
                          TxParseProcessor *processor) {
  TxNode *node = syntax->self;
  TxNode *parent = node->parent;

  // todo understand name vs scope_name vs content_name (for begin/end)
  if (syntax->content_name) {
    expand_name(syntax->content_name, state->matches[0].scope, state);
  } else if (syntax->name) {
    expand_name(syntax->name, state->matches[0].scope, state);
  }

#ifdef _DEBUG_COLLECT
  char_u *temp = state->matches[0].scope;
  printf("> %s {%s %s %s}\n", temp, syntax->name, syntax->scope_name, syntax->content_name);
  printf("+ (%d-%d) ", state->matches[0].start, state->matches[0].end);
  _BEGIN_COLOR(255, 255, 0)
  printf("%s", tx_extract_buffer_range(state->matches[0].buffer,
                                       state->matches[0].start,
                                       state->matches[0].end));
  _END_FORMAT
  printf(" <<<<< \n");
#endif
}

static void collect_captures(char_u *anchor, TxMatch *state,
                             TxSyntaxNode *captures,
                             TxParseProcessor *processor) {
  char_u temp[TX_SCOPE_NAME_LENGTH];

  char_u *capture_key[8];
  for (int j = 0; j < TX_MAX_MATCHES; j++) {
    int capture_idx = j;
    sprintf(capture_key, "%d", capture_idx);
    TxMatchRange *m = &state->matches[capture_idx];

    TxNode *n = txn_get(captures, capture_key);
    if (n) {
      TxNode *scope = txn_get(n, "name");
      if (scope && m->start >= 0) {
        expand_name(scope->string_value, temp, state);
        strncpy(m->scope, temp, TX_SCOPE_NAME_LENGTH);
#ifdef _DEBUG_COLLECT
        printf(":: %d (%d-%d) %s [", capture_idx, m->start, m->end, temp);
        _BEGIN_COLOR(0, 255, 255)
        printf("%s", tx_extract_buffer_range(anchor, m->start, m->end));
        _END_FORMAT
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

  if (processor) {
    processor->parser_state = stack;
  }
  if (processor && processor->line_start) {
    processor->line_start(processor, buffer_start, buffer_end);
  }

  TxSyntax *last_syntax = NULL;


  while (true) {
    TxMatch *top = tx_state_top(stack);
    TxSyntax *syntax = top->syntax;

    TxMatch pattern_match;
    tx_init_match(&pattern_match);
    TxMatch end_match;
    tx_init_match(&end_match);
    TxMatch while_match;
    tx_init_match(&while_match);

#ifdef _DEBUG_COLLECT
    printf("syntax: %x\n", syntax);
    printf("stack: %d offset: %d\n", stack->size, start - buffer_start);
#endif

    end = buffer_end;

    if (syntax->patterns) {
      pattern_match = match_first_pattern(anchor, start, end, syntax->patterns);
    }

    if (syntax->rx_end || syntax->rx_end_dynamic) {
      end_match = match_end(anchor, start, end, syntax);
    }

    if (syntax->rx_while) {
      if (find_match(anchor, start, end, syntax->rx_while, syntax->rxs_while, &while_match)) {
        while_match.syntax = syntax;
      }
      if (!while_match.syntax) {
        if (processor && processor->close_tag) {
          processor->close_tag(processor, top);
        }
        tx_state_pop(stack);
        continue;
      }
    }

    if (end_match.syntax &&
        (!pattern_match.syntax ||
         pattern_match.matches[0].start >= end_match.matches[0].start)) {
      pattern_match = end_match;

      end = buffer_start + pattern_match.matches[0].end;
      start = buffer_start + pattern_match.matches[0].start;

      tx_state_pop(stack);

      if (processor && processor->close_tag) {
        processor->close_tag(processor, &pattern_match);
      }
      collect_match(pattern_match.syntax, &pattern_match, processor);

      if (pattern_match.syntax->end_captures) {
        collect_captures(anchor, &pattern_match,
                         pattern_match.syntax->end_captures, processor);
      }

#ifdef _DEBUG_COLLECT
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

        if (processor && processor->open_tag) {
          processor->open_tag(processor, &pattern_match);
        }

#ifdef _DEBUG_COLLECT
        printf("{{{{\n");
        printf(tx_extract_buffer_range(anchor, pattern_match.matches[0].start,
                                       pattern_match.matches[0].end));
        printf("\n");
#endif
        if (pattern_match.syntax->begin_captures) {

          collect_captures(anchor, &pattern_match,
                           pattern_match.syntax->begin_captures, processor);
        }

      } else if (pattern_match.syntax->rx_match) {

        if (processor && processor->capture) {
          processor->capture(processor, &pattern_match);
        }

        if (pattern_match.syntax->captures) {

          collect_captures(anchor, &pattern_match,
                           pattern_match.syntax->captures, processor);
        }
      }
    }

    // prevent possible endless loop
    if (start == end && last_syntax == tx_state_top(stack)->syntax) {
      break;
    }

    start = end;
    last_syntax = tx_state_top(stack)->syntax;
  }

  if (processor && processor->line_end) {
    processor->line_end(processor);
  }
}
