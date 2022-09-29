#include "cJSON.h"
#include "munit.h"
#include "textmate.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4127)
#endif

#define EXTENSIONS_PATH "./tests/data/extensions"
#define _PRINT_CHECK(lead, tail) printf("%s\xE2\x9C\x94%s", lead, tail);
#define _PRINT_CROSS(lead, tail) printf("%s\xE2\x9C\x95%s", lead, tail);

static MunitResult test_syntax(const MunitParameter params[], void *data) {
  (void)params;
  (void)data;

  char *path = munit_parameters_get(params, "path");

  tx_read_package_dir(EXTENSIONS_PATH);

  TxNode *packages = tx_global_packages();
  munit_assert_not_null(packages);
  munit_assert_size(tx_global_packages()->size, >, 0);

  TxSyntaxNode *syntax_node = tx_syntax_from_path(path);

  TxSyntax *syntax = txn_syntax_value(syntax_node);
  munit_assert_not_null(syntax);
  munit_assert_not_null(syntax->patterns);
  munit_assert_size(((TxNode *)(syntax->patterns))->size, >, 0);
  munit_assert_not_null(syntax->repository);

  {
    TxParserState stack;
    tx_init_parser_state(&stack, syntax);

    TxParseProcessor processor;
    tx_init_processor(&processor, TxProcessorTypeCollect);

    char spec_path[1024];
    sprintf(spec_path, "%s.spec.json", path);
    TxNode *spec = txn_load_json(spec_path);

    _BEGIN_COLOR(255, 255, 0)
    printf("---[ %s ]---\n", path);
    _END_FORMAT

    char temp[TX_MAX_LINE_LENGTH];
    FILE *fp = fopen(path, "r");
    int row = 1;
    while (!feof(fp)) {
      strcpy(temp, "");
      fgets(temp, TX_MAX_LINE_LENGTH, fp);
      int len = strlen(temp);
      // printf("%s", temp);
      tx_parse_line(temp, temp + len + 1, &stack, &processor);

      char nz[32];
      sprintf(nz, "%d", row);
      TxNode *r = txn_get(spec, nz);
      if (r) {
        for (int col = 1; col < len; col++) {
          sprintf(nz, "%d", col);
          TxNode *c = txn_get(r, nz);
          if (c) {
            TxNode *text = txn_get(c, "text");
            munit_assert_not_null(strstr(temp + (col - 1), text->string_value));

            _BEGIN_COLOR(0, 255, 255)
            printf("%s\n", text->string_value);
            _END_FORMAT

            TxNode *scopes = txn_get(c, "scopes");
            TxNode *vscode_scopes = txn_get(c, "vscode_scopes");
            TxNode **test_scopes[] = {scopes, vscode_scopes};

            for (int k = 0; k < 2; k++) {
              TxNode *scopes = test_scopes[k];
              if (!scopes || !scopes->size)
                continue;

              TxNode *ch = scopes->first_child;
              while (ch) {
                bool scope_found = false;
                for (int j = 0;
                     j < processor.line_parser_state.size && !scope_found;
                     j++) {
                  TxMatch *state = &processor.line_parser_state.states[j];
                  for (int i = 0; i < state->size && !scope_found; i++) {
                    if (!state->matches[i].scope[0])
                      continue;
                    if (state->matches[i].start < 0 ||
                        state->matches[i].end < 0)
                      continue;
                    if (state->matches[i].start > col ||
                        state->matches[i].end < col) {
                      continue;
                    }
                    if (strcmp(state->matches[i].scope, ch->string_value) ==
                        0) {
                      scope_found = true;
                    }
                  }
                }

                if (scope_found) {
                  _BEGIN_COLOR(0, 255, 0)
                  _PRINT_CHECK("", " ")
                } else {
                  if (k == 0) {
                    _BEGIN_COLOR(255, 0, 0)
                  } else {
                    _BEGIN_COLOR(255, 255, 0)
                  }
                  _PRINT_CROSS("", " ")
                }
                _END_FORMAT
                printf("%s %s\n", ch->string_value, k == 1 ? "[vscode]" : "");

                if (k == 0) {
                  munit_assert_true(scope_found);
                }
                ch = ch->next_sibling;
              }
            }
          }
        }
      }

      row++;
    }
    fclose(fp);
  }

  return MUNIT_OK;
}

static MunitResult test_packages(const MunitParameter params[], void *data) {
  (void)params;
  (void)data;

  tx_read_package_dir(EXTENSIONS_PATH);

  TxNode *packages = tx_global_packages();
  munit_assert_not_null(packages);
  munit_assert_size(tx_global_packages()->size, >, 0);

  munit_assert_not_null(tx_syntax_from_scope("source.c"));
  munit_assert_not_null(tx_syntax_from_path("Makefile"));
  munit_assert_not_null(tx_syntax_from_path("test.html"));

  return MUNIT_OK;
}

static MunitResult test_theme(const MunitParameter params[], void *data) {
  (void)params;
  (void)data;

  return MUNIT_OK;
}

static void *test_setup(const MunitParameter params[], void *user_data) {
  (void)params;
  tx_initialize();
  munit_assert_string_equal(user_data, "µnit");
  return (void *)(uintptr_t)0xdeadbeef;
}

static void test_tear_down(void *fixture) {
  tx_shutdown();
  munit_assert_ptr_equal(fixture, (void *)(uintptr_t)0xdeadbeef);
}

static char *test_syntax_paths[] = {(char *)"./tests/data/main.c",
                                    (char *)"./tests/data/printf.c",
                                    (char *)"./tests/data/hello.vue",
                                    (char *)"./tests/data/javascript.js",
                                    (char *)"./tests/data/math.html",
                                    (char *)"./tests/data/includes.md",
                                    NULL};
static MunitParameterEnum syntax_test_params[] = {
    {(char *)"path", test_syntax_paths}, {NULL, NULL}};

static MunitTest test_suite_tests[] = {
    {(char *)"/tests/packages", test_packages, test_setup, test_tear_down,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/tests/syntax", test_syntax, test_setup, test_tear_down,
     MUNIT_TEST_OPTION_NONE, syntax_test_params},
    {(char *)"/tests/theme", test_theme, test_setup, test_tear_down,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {(char *)"", test_suite_tests, NULL,
                                      MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv) {
  TX_TIMER_BEGIN

  munit_suite_main(&test_suite, (void *)"µnit", argc, argv);
  tx_stats();

  TX_TIMER_END
  printf("\ntests done at %fsecs\n", _cpu_time_used);
  return 0;
}