#ifndef TEXTMATE_H
#define TEXTMATE_H

#include "onigmognu.h"
#include <stdbool.h>
#include <stdint.h>

#define TX_COLORIZE
#define TX_SYNTAX_VERBOSE_REGEX
#define TX_SYNTAX_RECOMPILE_REGEX_END

#define TX_MAX_STACK_DEPTH 64    // json, xml could be very deep
#define TX_MAX_MATCHES 100       // unbelievably .. cpp has a max of 81 captures
#define TX_SCOPE_NAME_LENGTH 128 //
#define TX_CAPTURED_NAME_LENGTH 32 //
#define TX_MAX_LINE_LENGTH 1024
#define TX_MAX_STYLE_SPANS 128

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
  TxObjectSyntax,
  TxObjectPackage,
  TxObjectTheme,
  TxObjectFontStyle,
} TxObjectType;

typedef struct _TxNode {
  TxValueType type;
  int32_t object_type;

  // tree
  struct _TxNode *parent;
  struct _TxNode *prev_sibling;
  struct _TxNode *next_sibling;
  struct _TxNode *first_child;
  struct _TxNode *last_child;
  size_t size;

  // data
  char *name;
  int32_t number_value;
  double double_value;
  char *string_value;
  void *data;

  void (*destroy)(struct _TxNode *);
} TxNode;

typedef struct _TxSyntax {
  TxNode *self;
  TxNode *root;
  TxNode *repository;

  TxNode *patterns;
  TxNode *include;
  TxNode *captures;
  TxNode *begin_captures;
  TxNode *end_captures;
  TxNode *while_captures;

  char *name;
  char *content_name;
  char *scope_name;

  bool include_external;
  char *include_scope;

  regex_t *rx_match;
  regex_t *rx_begin;
  regex_t *rx_end;
  regex_t *rx_while;

  char *rxs_match;
  char *rxs_begin;
  char *rxs_end;
  char *rxs_while;
  bool rx_end_dynamic;

  char *last_anchor;
  char *last_start;
  char *last_end;
  bool last_fail;

} TxSyntax;

typedef struct {
  TxNode self;
  TxSyntax syntax;
} TxSyntaxNode;

typedef struct {
  TxNode *self;
  TxNode *grammars;
  TxNode *languages;
  TxNode *themes;
} TxPackage;

typedef struct {
  TxNode self;
  TxPackage package;
} TxPackageNode;

typedef struct {
  TxNode *self;
  uint32_t fg;
  uint32_t bg;
  bool italic;
  bool bold;
  bool underline;
} TxFontStyle;

typedef struct {
  TxNode self;
  TxFontStyle style;
} TxFontStyleNode;

typedef struct {
  TxNode *self;
  TxNode *token_colors;
  TxNode *unresolved_scopes;
} TxTheme;

typedef struct {
  TxNode self;
  TxTheme theme;
} TxThemeNode;

typedef struct {
  int32_t start;
  int32_t end;
  TxFontStyle font_style;
} TxStyleSpan;

typedef struct {
  char *buffer;
  int32_t start;
  int32_t end;
  char scope[TX_SCOPE_NAME_LENGTH];
  char captured_name[TX_CAPTURED_NAME_LENGTH];
} TxMatchRange;

typedef struct {
  int32_t start;
  int32_t end;
  size_t scope_size;
  size_t name_size;
  char *scope;
  char *captured_name;
} TxMatchRangeEx;

typedef struct {
  TxSyntax *syntax;
  size_t size;
  size_t rank;
  TxMatchRange matches[TX_MAX_MATCHES]; // todo rename to ranges
} TxMatch;

typedef struct {
  TxSyntax *syntax;
  size_t size;
  size_t rank;
  TxMatchRangeEx *matches;
} TxMatchEx;

typedef struct {
  size_t size;
  TxMatch states[TX_MAX_STACK_DEPTH]; // todo rename to matches
} TxParserState;

typedef struct {
  size_t size;
  TxMatchEx states;
} TxParserStateEx;

typedef enum {
  TxProcessorTypeNull,
  TxProcessorTypeDump,
  TxProcessorTypeCollect,
  TxProcessorTypeCollectAndDump,
  TxProcessorTypeCollectAndRender,
  TxProcessorTypeCollectAndStyle,
} TxProcessorType;

typedef struct _TxParseProcessor {
  TxParserState *parser_state;
  TxParserState line_parser_state;
  void (*line_start)(struct _TxParseProcessor *self, char *buffer_start,
                     char *buffer_end);
  void (*line_end)(struct _TxParseProcessor *self);
  void (*open_tag)(struct _TxParseProcessor *self, TxMatch *state);
  void (*close_tag)(struct _TxParseProcessor *self, TxMatch *state);
  void (*capture)(struct _TxParseProcessor *self, TxMatch *state);
  char *buffer_start;
  char *buffer_end;
  TxTheme *theme;
  bool render_html;
  int line_styles_size;
  TxStyleSpan line_styles[TX_MAX_STYLE_SPANS];
} TxParseProcessor;

TxNode *txn_new(char *name, TxValueType type);
TxNode *txn_new_number(int32_t number);
TxNode *txn_new_string(char *string);
TxNode *txn_new_array();
TxNode *txn_new_object();
TxNode *txn_new_value(void *data);
char *txn_set_name(TxNode *node, char *name);
void txn_free(TxNode *node);

int32_t txn_number_value(TxNode *node);
char *txn_string_value(TxNode *node);
TxNode *txn_object_value(TxNode *node);
TxNode *txn_array_value(TxNode *node);
void *txn_value(TxNode *node);
void txn_set_string_value(TxNode *node, char *string);

TxNode *txn_push(TxNode *node, TxNode *child);
TxNode *txn_pop(TxNode *node);
TxNode *txn_set(TxNode *node, char *key, TxNode *value);
TxNode *txn_get(TxNode *node, char *key);
TxNode *txn_child_at(TxNode *node, size_t idx);
TxNode *txn_insert_at(TxNode *node, size_t idx, TxNode *child);
TxNode *txn_remove(TxNode *node, size_t idx);
TxNode *txn_root(TxNode *node);

TxSyntaxNode *txn_new_syntax();
TxSyntaxNode *txn_load_syntax(char *path);
TxSyntaxNode *txn_load_syntax_data(char *data);
TxSyntax *txn_syntax_value(TxSyntaxNode *syn);

TxPackageNode *txn_new_package();
TxPackageNode *txn_load_package(char *path);
TxPackage *txn_package_value(TxPackageNode *pkn);

TxThemeNode *txn_new_theme();
TxThemeNode *txn_load_theme(char *path);
TxThemeNode *txn_load_theme_data(char *data);
TxTheme *txn_theme_value(TxThemeNode *pkn);
TxFontStyleNode *txn_new_font_style();
TxFontStyle *txn_font_style_value(TxFontStyleNode *pkn);

void tx_initialize();
void tx_shutdown();

TxNode *txn_load_json(char *path);

void tx_parse_line(char *buffer_start, char *buffer_end,
                   TxParserState *stack, TxParseProcessor *processor);
char *tx_extract_buffer_range(char *anchor, size_t start, size_t end);

void tx_init_match(TxMatch *match);
void tx_init_processor(TxParseProcessor *processor, TxProcessorType type);
void tx_init_parser_state(TxParserState *stack, TxSyntax *syntax);
void tx_state_push(TxParserState *stack, TxMatch *match);
void tx_state_pop(TxParserState *stack);
TxMatch *tx_state_top(TxParserState *stack);

TxSyntaxNode *tx_global_repository();
TxNode *tx_global_packages();
void tx_read_package_dir(char *path);

TxSyntaxNode *tx_syntax_from_path(char *path);
TxSyntaxNode *tx_syntax_from_scope(char *scope);
TxThemeNode *tx_theme_from_name(char *name);
bool tx_style_from_scope(char *scope, TxTheme *theme, TxStyleSpan *style);

regex_t *tx_compile_pattern(char *pattern);

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

#ifdef TX_COLORIZE
#define _BEGIN_COLOR(R, G, B) printf("\x1b[38;2;%d;%d;%dm", R, G, B);
#define _BEGIN_BOLD printf("\x1b[1m");
#define _BEGIN_DIM printf("\x1b[2m");
#define _BEGIN_UNDERLINE printf("\x1b[4m");
#define _BEGIN_REVERSE printf("\x1b[7m");
#define _END_FORMAT printf("\x1b[0m");
#else
#define _BEGIN_COLOR(R, G, B)
#define _BEGIN_BOLD
#define _BEGIN_DIM
#define _BEGIN_UNDERLINE
#define _BEGIN_REVERSE
#define _END_FORMAT
#endif

#define TX_LOG printf

bool txt_parse_color(const char *color, uint32_t *result);
bool txt_color_to_rgb(uint32_t color, uint32_t result[3]);
uint32_t txt_make_color(int r, int g, int b);

void tx_stats();

#endif // TEXTMATE_H