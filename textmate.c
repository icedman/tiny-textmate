#include "textmate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TxSyntaxNode *global_repository = NULL;
static TxNode *global_packages = NULL;

#define TXSR_BUCKET_INCREMENTS 2500
static TxSyntax **global_syntax_registry = NULL;
static int32_t gsr_bucket = 0;
static int32_t gsr_count = 0;

regex_t *tx_compile_pattern(char_u *pattern) {
  regex_t *regex;
  OnigErrorInfo einfo;
  int result = onig_new(&regex, pattern, pattern + strlen((char *)pattern),
                        ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII,
                        ONIG_SYNTAX_DEFAULT, &einfo);
  if (result != ONIG_NORMAL) {
    printf("pattern not compiled %s\n", pattern);
  }
  return regex;
}

static uint32_t _nodes_allocated = 0;
static uint32_t _nodes_freed = 0;
static uint32_t _allocated = 0;
static uint32_t _freed = 0;

static void *tx_malloc_default(size_t size) {
  void *result = malloc(size);
  _allocated++;
  if (size > 0 && !result) {
    fprintf(stderr, "textmate failed to allocate %zu bytes", size);
    exit(1);
  }
  return result;
}

static void tx_free_default(void *data) { _freed++; free(data); }

void *(*tx_malloc)(size_t) = tx_malloc_default;
void (*tx_free)(void *) = tx_free_default;

void tx_set_allocator(void *(*custom_malloc)(size_t),
                      void (*custom_free)(void *)) {
  tx_malloc = custom_malloc;
  tx_free = custom_free;
}

TxNode *txn_new(char_u *name, TxValueType type) {
  TxNode *node = tx_malloc(sizeof(TxNode));
  memset(node, 0, sizeof(TxNode));
  node->type = type;
  _nodes_allocated++;
  txn_set_name(node, name);
  return node;
}

char_u *txn_set_name(TxNode *node, char_u *name) {
  if (node->name) {
    tx_free(node->name);
    node->name = NULL;
  }
  if (name) {
    size_t len = strlen(name);
    node->name = tx_malloc(sizeof(char_u) * (len + 1));
    strcpy((char_u *)node->name, name);
  }
  return node->name;
}

TxNode *txn_new_number(int32_t number) {
  TxNode *n = txn_new(NULL, TxTypeNumber);
  n->number_value = number;
  return n;
}

TxNode *txn_new_string(char_u *string) {
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

int32_t txn_number_value(TxNode *node) { return node->number_value; }

char_u *txn_string_value(TxNode *node) { return (char_u *)node->string_value; }

void txn_set_string_value(TxNode *node, char_u *string) {
  if (node->string_value) {
    tx_free(node->string_value);
    node->string_value = NULL;
  }
  if (string) {
    size_t len = strlen(string);
    node->string_value = tx_malloc(sizeof(char_u) * (len + 1));
    strcpy((char_u *)node->string_value, string);
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
  tx_free(node);

  // arrays and objects have children to be freed
  TxNode *child = node->first_child;
  while (child) {
    TxNode *next_sibling = child->next_sibling;
    txn_free(child);
    child = next_sibling;
  }
}

TxNode *txn_push(TxNode *node, TxNode *child) {
  child->index = node->size;
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
  while (child) {
    if (child->index == idx) {
      return child;
    }
    child = child->next_sibling;
  }
  return NULL;
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

TxNode *txn_set(TxNode *node, char_u *key, TxNode *value) {
  txn_set_name(value, key);
  txn_push(node, value);
  return value;
}

TxNode *txn_get(TxNode *node, char_u *key) {
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
  TxSyntax *syntax = (TxSyntax *)syntax_node->data;
  // free all syntax allocs here

  regex_t **regexes[] = {&syntax->rx_first_line_match,
                         &syntax->rx_folding_start_marker,
                         &syntax->rx_folding_Stop_marker,
                         &syntax->rx_match,
                         &syntax->rx_begin,
                         &syntax->rx_end,
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

  txsr_unregister(syntax);
  tx_free(syntax);
  syntax_node->data = NULL;
}

TxSyntaxNode *txn_new_syntax() {
  TxSyntaxNode *node = txn_new_object();
  node->object_type = TX_TYPE_SYNTAX;

  TxSyntax *syntax = tx_malloc(sizeof(TxSyntax));
  memset(syntax, 0, sizeof(TxSyntax));
  syntax->self = node;
  node->data = (void *)syntax;
  node->destroy = destroy_syntax;

  txsr_register(syntax);
  return node;
}

TxSyntax *txn_syntax_value(TxSyntaxNode *syn) {
  return syn->object_type == TX_TYPE_SYNTAX ? (TxSyntax *)syn->data : NULL;
}

uint32_t txsr_register(TxSyntax* syntax)
{
#ifdef TX_ENABLE_SERIALIZATION
  // resize
  if (gsr_count == gsr_bucket) {
    gsr_bucket += TXSR_BUCKET_INCREMENTS;
    TxSyntax *n = tx_malloc(sizeof(TxSyntax*) * gsr_bucket);
    if (gsr_count && global_syntax_registry) {
      memcpy(n, global_syntax_registry, sizeof(TxSyntax*) * gsr_count);
      tx_free(global_syntax_registry);
    }
    global_syntax_registry = n;
  }

  global_syntax_registry[gsr_count] = syntax;
  syntax->registry_id = 0xff + gsr_count++;
  // printf("%d\n", syntax->registry_id);
#endif
  return syntax->registry_id;
}

void txsr_unregister(TxSyntax* syntax)
{
#ifdef TX_ENABLE_SERIALIZATION
  global_syntax_registry[syntax->registry_id - 0xff] = 0;
#endif
}

TxSyntax* txsr_syntax(uint32_t id)
{
#ifdef TX_ENABLE_SERIALIZATION
  if (id - 0xff >= gsr_count) {
    return NULL;
  }
  return global_syntax_registry[id - 0xff];
#else
  return NULL;
#endif
}

TxPackageNode *txn_new_package() {
  TxPackageNode *node = tx_malloc(sizeof(TxPackageNode));
  memset(node, 0, sizeof(TxPackageNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectPackage;
  return node;
}

TxPackage *txn_package_value(TxPackageNode *node) {
  return node->self.object_type == TxObjectPackage ? &node->package : NULL;
}

TxThemeNode *txn_new_theme() {
  TxThemeNode *node = tx_malloc(sizeof(TxThemeNode));
  memset(node, 0, sizeof(TxThemeNode));
  node->self.type = TxTypeObject;
  node->self.object_type = TxObjectTheme;
  return node;
}

TxTheme *txn_theme_value(TxThemeNode *node) {
  return node->self.object_type == TxObjectTheme ? &node->theme : NULL;
}

void txs_init_stack(TxStateStack *stack) {
  memset(stack, 0, sizeof(TxStateStack));
}

void txs_init_state(TxState *state) { memset(state, 0, sizeof(TxState)); }

void txs_push(TxStateStack *stack, TxState *state) {
  // replace last on the stack
  if (stack->size >= TS_MAX_STACK_DEPTH - 1) {
    stack->size--;
  }
  memcpy(&stack->states[stack->size], state, sizeof(TxState));
  stack->size++;
}

void txs_pop(TxStateStack *stack) {
  if (stack->size > 0) {
    stack->size--;
  } else {
    // error!
  }
}

TxState *txs_top(TxStateStack *stack) {
  if (stack->size == 0) {
    return NULL;
  }
  return &stack->states[stack->size - 1];
}

void tx_initialize() {
  OnigEncoding use_encs[1];
  use_encs[0] = ONIG_ENCODING_ASCII;
  onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));
  tx_global_repository();
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
  if (global_syntax_registry) {
    tx_free(global_syntax_registry);
    global_syntax_registry = NULL;
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

void tx_stats() {
  printf("-----\nnodes allocated: %d\nnodes freed: %d\n", _nodes_allocated, _nodes_freed);
  printf("-----\nallocated: %d\nfreed: %d\n", _allocated, _freed);
}