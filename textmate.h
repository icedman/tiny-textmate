#ifndef TEXTMATE_H
#define TEXTMATE_H

#include "onigmognu.h"
#include <stdbool.h>
#include <stdint.h>

// #define TX_SYNTAX_VERBOSE_REGEX
// #define TX_SYNTAX_RECOMPILE_REGEX_END

#define TX_MAX_STACK_DEPTH 32
#define TX_MAX_PATTERN_DEPTH 16
#define TX_MAX_MATCHES 9
#define TX_SCOPE_NAME_LENGTH 128

#ifndef char_u
typedef uint8_t char_u;
#endif

extern void *(*tx_malloc)(size_t);
extern void (*tx_free)(void *);

void tx_set_allocator(void *(*custom_malloc)(size_t),
                      void (*custom_free)(void *));

typedef enum {
  TxTypeNull,
  TxTypeValue,
  TxTypeNumber,
  TxTypeString,
  TxTypeObject,
  TxTypeArray,
} TxValueType;

typedef enum {
  TxObjectNull,
  TxObjectPackage,
  TxObjectTheme,
  TxObjectSyntax,
} TxObjectType;

typedef struct _TxNode {
  TxValueType type;
  int32_t object_type;
  int32_t index;

  // tree
  struct _TxNode *parent;
  struct _TxNode *prev_sibling;
  struct _TxNode *next_sibling;
  struct _TxNode *first_child;
  struct _TxNode *last_child;
  size_t size;

  // data
  char_u *name;
  int32_t number_value;
  double double_value;
  char_u *string_value;
  void *data;

  void (*destroy)(struct _TxNode *);
} TxNode;

typedef struct _TxSyntax {
  struct TxNode *self;
  struct TxSyntaxNode *repository;

  struct TxSyntaxNode *patterns;
  struct TxSyntaxNode *include;
  struct TxSyntaxNode *captures;
  struct TxSyntaxNode *begin_captures;
  struct TxSyntaxNode *end_captures;

  char_u *name;
  char_u *content_name;
  char_u *scope_name;

  bool include_external;

  regex_t *rx_match;
  regex_t *rx_begin;
  regex_t *rx_end;

  char_u *rxs_match;
  char_u *rxs_begin;
  char_u *rxs_end;

} TxSyntax;

typedef struct {
  TxNode self;
  TxSyntax syntax;
} TxSyntaxNode;

typedef struct {
  struct TxNode *self;
  TxNode *grammars;
  TxNode *languages;
  TxNode *themes;
} TxPackage;

typedef struct {
  TxNode self;
  TxPackage package;
} TxPackageNode;

typedef struct {
  struct TxNode *self;
  struct TxNode *token_colors;
} TxTheme;

typedef struct {
  TxNode self;
  TxTheme theme;
} TxThemeNode;

typedef struct {
  size_t start;
  size_t end;
  uint32_t fg;
  uint32_t bg;
  bool italic;
  bool bold;
  bool underline;
} TxStyleSpan;

typedef struct {
  char_u *buffer;
  size_t start;
  size_t end;
  char_u scope[TX_SCOPE_NAME_LENGTH];
} TxMatchRange;

typedef struct {
  TxSyntax *syntax;
  size_t size;
  TxMatchRange matches[TX_MAX_MATCHES]; // todo rename to ranges
} TxMatch;

typedef struct {
  size_t size;
  TxMatch states[TX_MAX_STACK_DEPTH]; // todo rename to matches
} TxParserState;

typedef enum {
  TxProcessorTypeNull,
  TxProcessorTypeDump,
  TxProcessorTypeCollect,
  TxProcessorTypeCollectAndDump,
  TxProcessorTypeCollectAndRender,
} TxProcessorType;

typedef struct {
  TxParserState *parser_state;
  TxParserState line_parser_state;
  void (*line_start)(struct TxParseProcessor *self, char_u *buffer_start,
                     char_u *buffer_end);
  void (*line_end)(struct TxParseProcessor *self);
  void (*open_tag)(struct TxParseProcessor *self, TxMatch *state);
  void (*close_tag)(struct TxParseProcessor *self, TxMatch *state);
  void (*capture)(struct TxParseProcessor *self, TxMatch *state);
  char_u *buffer_start;
  char_u *buffer_end;
  TxTheme *theme;
} TxParseProcessor;

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
void txn_set_string_value(TxNode *node, char_u *string);

TxNode *txn_push(TxNode *node, TxNode *child);
TxNode *txn_pop(TxNode *node);
TxNode *txn_set(TxNode *node, char_u *key, TxNode *value);
TxNode *txn_get(TxNode *node, char_u *key);
TxNode *txn_child_at(TxNode *node, size_t idx);
TxNode *txn_root(TxNode *node);

TxSyntaxNode *txn_new_syntax();
TxSyntaxNode *txn_load_syntax(char_u *path);
TxSyntax *txn_syntax_value(TxSyntaxNode *syn);

TxPackageNode *txn_new_package();
TxPackageNode *txn_load_package(char_u *path);
TxPackage *txn_package_value(TxPackageNode *pkn);

TxThemeNode *txn_new_theme();
TxThemeNode *txn_load_theme(char_u *path);
TxTheme *txn_theme_value(TxThemeNode *pkn);

void tx_initialize();
void tx_shutdown();

void tx_parse_line(char_u *buffer_start, char_u *buffer_end,
                   TxParserState *stack, TxParseProcessor *processor);
char_u *tx_extract_buffer_range(char_u *anchor, size_t start, size_t end);

void tx_init_match(TxMatch *match);
void tx_init_processor(TxParseProcessor *processor, TxProcessorType type);
void tx_init_parser_state(TxParserState *stack, TxSyntax *syntax);
void tx_state_push(TxParserState *stack, TxMatch *match);
void tx_state_pop(TxParserState *stack);
TxMatch *tx_state_top(TxParserState *stack);

TxSyntaxNode *tx_global_repository();
TxNode *tx_global_packages();
void tx_read_package_dir(char *path);

TxSyntaxNode *tx_syntax_from_path(char_u *path);
TxSyntaxNode *tx_syntax_from_scope(char_u *scope);
TxThemeNode *tx_theme_from_name(char_u *name);
bool tx_style_from_scope(char_u *scope, TxTheme *theme, TxStyleSpan *style);

regex_t *tx_compile_pattern(char_u *pattern);

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

#define _BEGIN_COLOR(R, G, B) printf("\x1b[38;2;%d;%d;%dm", R, G, B);
#define _END_COLOR printf("\x1b[0m");

bool txt_parse_color(const char_u *color, uint32_t *result);
bool txt_color_to_rgb(uint32_t color, uint32_t result[3]);
uint32_t txt_make_color(int r, int g, int b);

#endif // TEXTMATE_H