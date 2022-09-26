#include "onigmognu.h"
#include <stdbool.h>
#include <stdio.h>

regex_t *compile_pattern(char *pattern) {
  regex_t *regex;
  OnigErrorInfo einfo;
  int result = onig_new(&regex, pattern, pattern + strlen((char *)pattern),
                        ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII,
                        ONIG_SYNTAX_DEFAULT, &einfo);
  if (result != ONIG_NORMAL) {
    printf("pattern not compiled\n");
  }
  return regex;
}

static bool find_match(char *buffer_start, char *buffer_end, regex_t *regex) {
  bool found = false;

  OnigRegion *region = onig_region_new();
  int r;
  unsigned char *start, *range, *end;

  unsigned int onig_options = ONIG_OPTION_NONE;
  // onig_options |= ONIG_OPTION_NOTEOS;
  // onig_options |= ONIG_OPTION_NOTBOL;
  // onig_options |= ONIG_OPTION_NOTEOL;

  char temp[128] = "";
  UChar *str = (UChar *)buffer_start;
  end = buffer_end;
  start = str;
  range = end;
  r = onig_search(regex, str, end, start, range, region, onig_options);
  if (r != ONIG_MISMATCH) {
    for (int i = 0; i < region->num_regs; i++) {
      char temp[1024];
      int len = (region->end[i] - region->beg[i]);
      memcpy(temp, buffer_start + region->beg[i], len * sizeof(char));
      temp[len] = 0;
      printf("%d %d %s\n", region->beg[i], region->end[i], temp);
    }
    found = true;
  }

  onig_end();
  onig_region_free(region, 1);
  return found;
}

int main(int argc, char **argv) {
  OnigEncoding use_encs[1];
  use_encs[0] = ONIG_ENCODING_ASCII;
  onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));

  // char temp[] = "the lazy quick brown fox";
  // regex_t* rx = compile_pattern("(lazy)\\s(quick)");

  {
    char temp[] = "int argc, char **argv) {";
    printf("%s\n", temp);
    // regex_t* rx = compile_pattern("%|\\*|/|-|\\+");
    regex_t *rx = compile_pattern(
        "(?<=(?:[a-zA-Z_0-9] "
        "|[&*>\\]\\)]))\\s*([a-zA-Z_]\\w*)\\s*(?=(?:\\[\\]\\s*)?(?:,|\\)))");
    find_match(temp, temp + strlen(temp), rx);
    onig_free(rx);
  }

  return 0;
}