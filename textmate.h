#ifndef TEXTMATE_H
#define TEXTMATE_H

#include "onigmognu.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef char_u
typedef uint8_t char_u;
#endif

extern void *(*tx_malloc)(size_t);
extern void (*tx_free)(void *);

void tx_set_allocator(void *(*custom_malloc)(size_t), void (*custom_free)(void *));

#define TX_TYPE_NODE    0
#define TX_TYPE_SYNTAX  1
#define TX_TYPE_PACKAGE 2
#define TX_TYPE_THEME   3

typedef enum {
  TxTypeNull,
  TxTypeValue,
  TxTypeNumber,
  TxTypeString,
  TxTypeObject,
  TxTypeArray,
} TxValueType;

typedef struct _TxNode {
  TxValueType type;
  int32_t object_type;
  int32_t index;
  char *name;
  int32_t number_value;
  double double_value;
  char *string_value;
  void *data;
  struct _TxNode *parent;
  struct _TxNode *prev_sibling;
  struct _TxNode *next_sibling;
  struct _TxNode *first_child;
  struct _TxNode *last_child;
  size_t size;
  void (*destroy)(struct _TxNode *);
} TxNode;

typedef TxNode TxSyntaxNode;
typedef TxNode TxPackageNode;
typedef TxNode TxThemeNode;

typedef struct _TxSyntax {
  TxSyntaxNode *self;

  TxSyntaxNode *repository;

  TxSyntaxNode *patterns;
  TxSyntaxNode *include;
  TxSyntaxNode *captures;
  TxSyntaxNode *end_captures;

  char *name;
  char *content_name;
  char *scope_name;

  bool include_external;

  regex_t *rx_first_line_match;     // unused
  regex_t *rx_folding_start_marker; // unused
  regex_t *rx_folding_Stop_marker;  // unused
  regex_t *rx_match;
  regex_t *rx_begin;
  regex_t *rx_end;

  // config
  bool verbose;
} TxSyntax;

typedef struct {
  TxPackageNode *self;
  TxNode* grammars;
  TxNode* languages;
  TxNode* themes;
} TxPackage;

typedef struct {
  TxThemeNode *self;
} TxTheme;

#define TS_MAX_MATCHES 32
#define TS_MAX_STACK_DEPTH 16

typedef struct {
  size_t start;
  size_t end;
} TxMatchRange;

typedef struct {
  TxSyntax *syntax;
  uint8_t count;
  size_t offset;
  TxMatchRange matches[TS_MAX_MATCHES];
} TxState;

typedef struct {
  uint8_t size;
  TxState states[TS_MAX_STACK_DEPTH];
} TxStateStack;

TxNode *txn_new(char_u *name, TxValueType type);
TxNode *txn_new_number(int32_t number);
TxNode *txn_new_string(char_u *string);
TxNode *txn_new_array();
TxNode *txn_new_object();
TxNode *txn_new_value(void *data);
char_u *txn_set_name(TxNode *node, char_u *name);
void txn_free(TxNode *node);

int32_t txn_number_value(TxNode *node);
char_u *txn_string_value(TxNode *node);
TxNode *txn_object_value(TxNode *node);
TxNode *txn_array_value(TxNode *node);
void *txn_value(TxNode *node);
void txn_set_string_value(TxNode* node, char_u* string);

TxNode *txn_push(TxNode *node, TxNode *child);
TxNode *txn_pop(TxNode *node);
TxNode *txn_set(TxNode *node, char_u *key, TxNode *value);
TxNode *txn_get(TxNode *node, char_u *key);
TxNode *txn_child_at(TxNode *node, size_t idx);
TxNode *txn_root(TxNode *node);

TxSyntaxNode* txn_new_syntax();
TxSyntaxNode* txn_load_syntax(char_u* path);
TxSyntax* txn_syntax_value(TxSyntaxNode* syn);

TxPackageNode* txn_new_package();
TxPackageNode* txn_load_package(char_u* path);
TxPackage* txn_package_value(TxPackageNode* pkn);

TxThemeNode* txn_new_theme();
TxThemeNode* txn_load_theme(char_u* path);
TxTheme* txn_theme_value(TxThemeNode* pkn);

void tx_parse_line(char_u *buffer_start, char_u *buffer_end, TxStateStack *stack);

void txs_init_stack(TxStateStack *stack);
void txs_init_state(TxState *state);
void txs_push(TxStateStack *stack, TxState *state);
void txs_pop(TxStateStack *stack);
TxState* txs_top(TxStateStack *stack);

void tx_initialize();
void tx_shutdown();

TxSyntaxNode* tx_global_repository();
TxNode* tx_global_packages();
void tx_read_package_dir(char *path);

TxSyntaxNode *tx_syntax_from_path(char_u *path);
TxSyntaxNode *tx_syntax_from_scope(char_u *scope);

regex_t* tx_compile_pattern(char_u *pattern);

#define TX_TIMER_BEGIN                                                         \
  clock_t _start, _end;                                                        \
  double _cpu_time_used;                                                       \
  _start = clock();

#define TX_TIMER_RESET _start = clock();

#define TX_TIMER_END                                                           \
  _end = clock();                                                              \
  _cpu_time_used = ((double)(_end - _start)) / CLOCKS_PER_SEC;

#ifndef MAX_PATH_LENGTH
#define MAX_PATH_LENGTH 1024
#endif

#ifndef _WIN32
#define DIR_SEPARATOR '/'
#else
#define DIR_SEPARATOR '\\'
#endif

#endif // TEXTMATE_H