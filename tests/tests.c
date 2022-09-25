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

static MunitResult test_syntax(const MunitParameter params[], void *data) {
  (void)params;
  (void)data;

  tx_read_package_dir(EXTENSIONS_PATH);

  TxNode *packages = tx_global_packages();
  munit_assert_not_null(packages);
  munit_assert_size(tx_global_packages()->size, >, 0);

  TxSyntaxNode *syntax_node = tx_syntax_from_scope("source.c");

  TxSyntax* syntax = txn_syntax_value(syntax_node);
  munit_assert_not_null(syntax);
  munit_assert_not_null(syntax->patterns);
  munit_assert_size(((TxNode*)(syntax->patterns))->size, >, 0);
  munit_assert_not_null(syntax->repository);
  munit_assert_size(((TxNode*)(syntax->repository))->size, >, 0);

  {
    TxStateStack stack;
    TxState state;
    txs_init_stack(&stack);
    txs_init_state(&state);
    state.syntax = syntax;
    txs_push(&stack, &state);

    const char *line = "int main(int argc, char **argv)";
    // int
    // storage.type.built-in.primitive.c source.c
    // storage.type

    // main
    // entity.name.function.c meta.function.definition.paramters.c meta.function.c source.c
    // entity.name.function

    // argc
    // variable.parameter.probably.c entity.name.function.c meta.function.definition.paramters.c meta.function.c source.c
    // variable

    TxProcessor processor;
    txp_line_processor(&processor);
    tx_parse_line(line, line + strlen(line) + 1, &stack, &processor);

    for(int i=0; i<processor.state_stack.size; i++) {
      TxState *state = &processor.state_stack.states[i];
      printf("state: %d\n", i);
      for(int j=0; j<state->count; j++) {
        printf(">%s %s [%s]\n", state->matches[0].scope, state->matches[j].expanded, state->matches[0].name);
      }
    }
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

static void *test_setup(const MunitParameter params[],
                                void *user_data) {
  (void)params;
  tx_initialize();
  munit_assert_string_equal(user_data, "µnit");
  return (void *)(uintptr_t)0xdeadbeef;
}

static void test_tear_down(void *fixture) {
  tx_shutdown();
  munit_assert_ptr_equal(fixture, (void *)(uintptr_t)0xdeadbeef);
}

static MunitTest test_suite_tests[] = {
    {(char *)"/tests/packages", test_packages, test_setup,
     test_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/tests/syntax", test_syntax, test_setup,
     test_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
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