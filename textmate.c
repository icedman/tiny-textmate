#include "textmate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TxSyntaxNode *global_repository = NULL;
static TxNode *global_packages = NULL;

regex_t *tx_compile_pattern(char *pattern) {
  int len = strlen(pattern);
  regex_t *regex;
  OnigErrorInfo einfo;
  int result = onig_new(&regex, pattern, pattern + len, ONIG_OPTION_DEFAULT,
                        ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, &einfo);
  if (result != ONIG_NORMAL) {
    OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
    onig_error_code_to_str(s, result, &einfo);
    TX_LOG("onig error: %s %s\n", s, pattern);
    if (regex) {
      onig_free(regex);
      regex = NULL;
    }
  }
  return regex;
}

static uint32_t _nodes_allocated = 0;
static uint32_t _nodes_freed = 0;
static uint32_t _allocated = 0;
static uint32_t _freed = 0;
static size_t _total_state_size = 0;
size_t _parsed_lines = 0;
uint32_t _match_execs = 0;
uint32_t _match_skips = 0;

static void *tx_malloc_default(size_t size) {
  void *result = malloc(size);
  _allocated++;
  if (size > 0 && !result) {
    fprintf(stderr, "textmate failed to allocate %zu bytes", size);
    exit(1);
  }
  return result;
}

static void tx_free_default(void *data) {
  _freed++;
  free(data);
}

void *(*tx_malloc)(size_t) = tx_malloc_default;
void (*tx_free)(void *) = tx_free_default;

void tx_set_allocator(void *(*custom_malloc)(size_t),
                      void (*custom_free)(void *)) {
  tx_malloc = custom_malloc;
  tx_free = custom_free;
}

TxNode *txn_new(char *name, TxValueType type) {
  TxNode *node = tx_malloc(sizeof(TxNode));
  memset(node, 0, sizeof(TxNode));
  node->type = type;
  _nodes_allocated++;
  txn_set_name(node, name);
  return node;
}

char *txn_set_name(TxNode *node, char *name) {
  if (node->name) {
    tx_free(node->name);
    node->name = NULL;
  }
  if (name) {
    size_t len = strlen(name);
    node->name = tx_malloc(sizeof(char) * (len + 1));
    strcpy((char *)node->name, name);
  }
  return node->name;
}

TxNode *txn_new_number(int32_t number) {
  TxNode *n = txn_new(NULL, TxTypeNumber);
  n->number_value = number;
  return n;
}

TxNode *txn_new_string(char *string) {
  TxNode *n = txn_new(NULL, TxTypeString);
  txn_set_string_value(n, string);
  return n;
}

TxNode *txn_new_array() { return txn_new(NULL, TxTypeArray); }

TxNode *txn_new_object() { return txn_new(NULL, TxTypeObject); }

TxNode *txn_new_value(void *data) {
  TxNode *n = txn_new(NULL, TxTypeValue);
  n->data = data;
  return n;
}

int32_t txn_number_value(TxNode *node) { return node ? node->number_value : 0; }

char *txn_string_value(TxNode *node) {
  return node ? (char *)node->string_value : NULL;
}

void txn_set_string_value(TxNode *node, char *string) {
  if (!node)
    return;
  if (node->string_value) {
    tx_free(node->string_value);
    node->string_value = NULL;
  }
  if (string) {
    size_t len = strlen(string);
    node->string_value = tx_malloc(sizeof(char) * (len + 1));
    strcpy((char *)node->string_value, string);
  }
}

TxNode *txn_object_value(TxNode *node) {
  if (node->type != TxTypeObject) {
    return NULL;
  }
  return node;
}

TxNode *txn_array_value(TxNode *node) {
  if (node->type != TxTypeArray) {
    return NULL;
  }
  return node;
}

void *txn_value(TxNode *node) { return node->data; }

void txn_free(TxNode *node) {
  _nodes_freed++;
  if (node->destroy) {
    node->destroy(node);
  }
  if (node->name) {
    tx_free(node->name);
    node->name = NULL;
  }
  if (node->data) {
    tx_free(node->data);
    node->data = NULL;
  }
  if (node->string_value) {
    tx_free(node->string_value);
    node->string_value = NULL;
  }

  // arrays and objects have children to be freed
  TxNode *child = node->first_child;
  while (child) {
    TxNode *next_sibling = child->next_sibling;
    txn_free(child);
    child = next_sibling;
  }

  tx_free(node);
}

TxNode *txn_push(TxNode *node, TxNode *child) {
  child->parent = node;
  node->size++;
  if (node->first_child == NULL) {
    node->first_child = child;
    node->last_child = child;
    return child;
  }
  child->prev_sibling = node->last_child;
  node->last_child->next_sibling = child;
  node->last_child = child;
  return child;
}

TxNode *txn_pop(TxNode *node) {
  if (node->size == 0) {
    return NULL;
  }
  TxNode *last = node->last_child;
  if (node->size == 1) {
    node->first_child = NULL;
    node->last_child = NULL;
    node->size = 0;
    return last;
  }
  last->prev_sibling->next_sibling = NULL;
  node->last_child = last->prev_sibling;
  node->size--;

  return last;
}

TxNode *txn_child_at(TxNode *node, size_t idx) {
  TxNode *child = node->first_child;
  int _idx = 0;
  while (child) {
    if (_idx++ == idx) {
      return child;
    }
    child = child->next_sibling;
  }
  return NULL;
}

TxNode *txn_insert_at(TxNode *node, size_t idx, TxNode *child) {
  TxNode *sibling = txn_child_at(node, idx);
  if (!sibling || sibling == node->last_child) {
    return txn_push(node, child);
  }
  child->parent = node;
  child->next_sibling = sibling;
  child->prev_sibling = sibling->prev_sibling;
  if (sibling->prev_sibling) {
    sibling->prev_sibling->next_sibling = child;
  }
  sibling->prev_sibling = child;
  if (sibling == node->first_child) {
    node->first_child = child;
  }
  node->size++;
  return child;
}

TxNode *txn_remove(TxNode *node, size_t idx) {
  TxNode *child = txn_child_at(node, idx);
  if (child) {
    if (child->prev_sibling) {
      child->prev_sibling->next_sibling = child->next_sibling;
      if (child == node->last_child) {
        node->last_child = child->prev_sibling;
      }
    }
    if (child->next_sibling) {
      child->next_sibling->prev_sibling = child->prev_sibling;
      if (child == node->first_child) {
        node->first_child = child->next_sibling;
      }
    }
    node->size--;
    child->parent = NULL;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    if (node->size == 0) {
      node->first_child = NULL;
      node->last_child = NULL;
    }
  }
  return child;
}

TxNode *txn_root(TxNode *node) {
  TxNode *root = node;
  while (true) {
    if (!root->parent)
      break;
    root = root->parent;
  }
  return root;
}

TxNode *txn_set(TxNode *node, char *key, TxNode *value) {
  txn_set_name(value, key);
  txn_push(node, value);
  return value;
}

TxNode *txn_get(TxNode *node, char *key) {
  TxNode *child = node->first_child;
  while (child) {
    if (strcmp(child->name, key) == 0) {
      return child;
    }
    child = child->next_sibling;
  }
  return NULL;
}

static void destroy_syntax(TxNode *node) {
  TxSyntaxNode *syntax_node = (TxSyntaxNode *)node;
  TxSyntax *syntax = txn_syntax_value(syntax_node);

  regex_t **regexes[] = {&syntax->rx_match, &syntax->rx_begin, &syntax->rx_end,
                         -1};

  for (int i = 0;; i++) {
    if (regexes[i] == -1)
      break;
    if (regexes[i]) {
      regex_t *r = *regexes[i];
      onig_free(r);
      *regexes[i] = NULL;
    }
  }
}

TxSyntaxNode *txn_new_syntax() {
  TxSyntaxNode *node = tx_malloc(sizeof(TxSyntaxNode));
  _nodes_allocated++;
  memset(node, 0, sizeof(TxSyntaxNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectSyntax;
  node->self.destroy = destroy_syntax;
  node->syntax.self = node;
  return node;
}

TxSyntax *txn_syntax_value(TxSyntaxNode *node) {
  return node->self.object_type == TxObjectSyntax ? &node->syntax : NULL;
}

TxPackageNode *txn_new_package() {
  TxPackageNode *node = tx_malloc(sizeof(TxPackageNode));
  _nodes_allocated++;
  memset(node, 0, sizeof(TxPackageNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectPackage;
  node->package.self = node;
  return node;
}

TxPackage *txn_package_value(TxPackageNode *node) {
  return node->self.object_type == TxObjectPackage ? &node->package : NULL;
}

TxThemeNode *txn_new_theme() {
  TxThemeNode *node = tx_malloc(sizeof(TxThemeNode));
  _nodes_allocated++;
  memset(node, 0, sizeof(TxThemeNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectTheme;
  node->theme.self = node;

  node->theme.unresolved_scopes =
      txn_set(node, "unresolved_scopes", txn_new_array());
  node->theme.token_colors = txn_set(node, "tokenColors", txn_new_object());
  return node;
}

TxTheme *txn_theme_value(TxThemeNode *node) {
  return node->self.object_type == TxObjectTheme ? &node->theme : NULL;
}

TxFontStyleNode *txn_new_font_style() {
  TxFontStyleNode *node = tx_malloc(sizeof(TxFontStyleNode));
  _nodes_allocated++;
  memset(node, 0, sizeof(TxFontStyleNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectFontStyle;
  node->style.self = node;
  return node;
}

TxFontStyle *txn_font_style_value(TxFontStyleNode *node) {
  return node->self.object_type == TxObjectFontStyle ? &node->style : NULL;
}

void tx_initialize() {
  OnigEncoding use_encs[1];
  use_encs[0] = ONIG_ENCODING_UTF8;
  onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));

  tx_global_repository();
  tx_global_packages();
}

void tx_shutdown() {
  if (global_repository) {
    txn_free(global_repository);
    global_repository = NULL;
  }
  if (global_packages) {
    txn_free(global_packages);
    global_packages = NULL;
  }
}

TxSyntaxNode *tx_global_repository() {
  if (!global_repository) {
    global_repository = txn_new_object();
  }
  return global_repository;
}

TxNode *tx_global_packages() {
  if (!global_packages) {
    global_packages = txn_new_array();
  }
  return global_packages;
}

void tx_init_match(TxMatch *state) { memset(state, 0, sizeof(TxMatch)); }

void tx_init_parser_state(TxParserState *stack, TxSyntax *syntax) {
  memset(stack, 0, sizeof(TxParserState));
  if (syntax) {
    TxMatch state;
    tx_init_match(&state);
    state.syntax = syntax;
    tx_state_push(stack, &state);
  }
}

void tx_state_push(TxParserState *stack, TxMatch *state) {
  // replace last on the stack
  memcpy(&stack->states[stack->size], state, sizeof(TxMatch));
  stack->size++;

  // when so deep a nest - discard; but always retain the root
  if (stack->size == TX_MAX_STACK_DEPTH) {
    int len = stack->size >> 1;
    memcpy(&stack->states[1], &stack->states[stack->size - len],
           len * sizeof(TxMatch));
    stack->size = len + 1;
  }
}

void tx_state_pop(TxParserState *stack) {
  if (stack->size > 0) {
    stack->size--;
  } else {
    // error!
  }
}

TxMatch *tx_state_top(TxParserState *stack) {
  if (stack->size == 0) {
    return NULL;
  }
  return &stack->states[stack->size - 1];
}

static void destroy_parser_state(TxNode *node) {
  TxParserStateEx *parser_state = txn_parser_state_value(node);
  if (!parser_state->size)
    return;
  for (int i = 0; i < parser_state->size; i++) {
    TxMatchEx *mx = &parser_state->states[i];
    if (!mx->size)
      continue;
    for (int j = 0; j < mx->size; j++) {
      TxMatchRangeEx *mrx = &mx->matches[j];
      if (mrx->scope) {
        // printf("%s\n", mrx->scope);
        tx_free(mrx->scope);
      }
      if (mrx->captured_name) {
        // printf("%s\n", mrx->captured_name);
        tx_free(mrx->captured_name);
      }
    }
    tx_free(mx->matches);
  }
  tx_free(parser_state->states);
  parser_state->size = 0;
}

TxParserStateNode *txn_new_parser_state() {
  TxParserStateNode *node = tx_malloc(sizeof(TxParserStateNode));
  _nodes_allocated++;
  memset(node, 0, sizeof(TxParserStateNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectParserState;
  node->self.destroy = destroy_parser_state;
  node->parser_state.self = node;
  return node;
}

TxParserStateEx *txn_parser_state_value(TxParserStateNode *node) {
  return node->self.object_type == TxObjectParserState ? &node->parser_state
                                                       : NULL;
}

bool txn_parser_state_serialize(TxParserStateNode *node, TxParserState *value) {
  if (node->parser_state.size) {
    destroy_parser_state(node);
  }

  memset(&node->parser_state, 0, sizeof(TxParserStateEx));

  node->parser_state.size = value->size;
  node->parser_state.states = tx_malloc(sizeof(TxMatchEx) * value->size);
  memset(node->parser_state.states, 0, sizeof(TxMatchEx) * value->size);

  _total_state_size += sizeof(TxMatchEx) * value->size;

  for (int i = 0; i < value->size; i++) {
    TxMatch *m = &value->states[i];
    TxMatchEx *mx = &node->parser_state.states[i];
    mx->syntax = m->syntax;
    mx->size = m->size;
    if (!m->size)
      continue;
    mx->matches = tx_malloc(sizeof(TxMatchRangeEx) * m->size);

    _total_state_size += sizeof(TxMatchRangeEx) * m->size;

    memset(mx->matches, 0, sizeof(TxMatchRangeEx) * m->size);
    for (int j = 0; j < m->size; j++) {
      TxMatchRange *mr = &m->matches[j];
      TxMatchRangeEx *mrx = &mx->matches[j];
      mrx->start = mr->start;
      mrx->end = mr->end;
      size_t len = mr->scope ? strlen(mr->scope) : 0;
      if (len) {
        mrx->scope = tx_malloc(sizeof(char) * len);
        _total_state_size += sizeof(char) * len;
        strcpy(mrx->scope, mr->scope);
      }
      if (m->syntax->rx_end_dynamic && mr->captured_name) {
        size_t len = strlen(mr->captured_name);
        if (len) {
          mrx->captured_name = tx_malloc(sizeof(char) * len);
          _total_state_size += sizeof(char) * len;
          strcpy(mrx->captured_name, mr->captured_name);
        }
      }
    }
  }

  return true;
}

bool txn_parser_state_unserialize(TxParserStateNode *node,
                                  TxParserState *value) {
  if (!node->parser_state.size) {
    return false;
  }

  memset(value, 0, sizeof(TxParserState));

  value->size = node->parser_state.size;
  for (int i = 0; i < value->size; i++) {
    TxMatch *m = &value->states[i];
    TxMatchEx *mx = &node->parser_state.states[i];
    m->syntax = mx->syntax;
    m->size = mx->size;
    if (!m->size)
      continue;
    for (int j = 0; j < m->size; j++) {
      TxMatchRange *mr = &m->matches[j];
      TxMatchRangeEx *mrx = &mx->matches[j];
      mr->start = mrx->start;
      mr->end = mrx->end;
      if (mrx->scope) {
        strncpy(mr->scope, mrx->scope, TX_SCOPE_NAME_LENGTH);
      }
      if (mrx->captured_name) {
        strncpy(mr->scope, mrx->captured_name, TX_SCOPE_NAME_LENGTH);
      }
    }
  }

  return true;
}

void tx_stats() {
  TX_LOG("-----\nnodes allocated: %d\nnodes freed: %d\n", _nodes_allocated,
         _nodes_freed);
  TX_LOG("-----\nallocated: %d\nfreed: %d\n", _allocated, _freed);
  TX_LOG("parsed lines: %d\n", _parsed_lines);
  TX_LOG("regex matches: %d\n", _match_execs);
  TX_LOG("regex skipped: %d\n", _match_skips);
  TX_LOG("total parser state size: %.2fMB\n",
         (float)_total_state_size / 1000000);
}