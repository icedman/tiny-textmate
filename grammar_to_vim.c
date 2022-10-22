#include "cJSON.h"
#include "textmate.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_STRING_LENGTH (1024 * 4)
static char temp_string[MAX_STRING_LENGTH];

char* cleanup_regex(char* regex) {
  int l = strlen(regex);
  if (l > MAX_STRING_LENGTH) {
    printf("regex length is too long\n");
    exit(0);
  }
  char c = '/';
  int idx = 0;

  temp_string[idx++] = c;
  // temp_string[idx++] = 'o';
  // temp_string[idx++] = 'n';
  // temp_string[idx++] = 'i';
  // temp_string[idx++] = 'g';
  // temp_string[idx++] = ':';

  for(int i=0; i<l; i++) {
    if (regex[i] == '\n') {
      goto cont;
    }
    if (regex[i] == '|' || regex[i] == '(' || regex[i] == ')' || regex[i] == '{' || regex[i] == '}') {
      temp_string[idx++] = '\\';
    }
    if (regex[i] == '\\' && regex[i+1] == 'b') {
      i++;
      goto cont;
    }
    temp_string[idx++] = regex[i];
    cont:
    temp_string[idx] = c;
    temp_string[idx+1] = 0;
  }
  return temp_string;
}

char* cleanup_name(char* name) {
  int l = strlen(name);
  if (l > MAX_STRING_LENGTH) {
    printf("name length is too long\n");
    exit(0);
  }
  int idx = 0;
  for(int i=0; i<l; i++) {
    if (name[i] == '.' || name[i] == '-') {
      temp_string[idx++] = '_';
    } else {
      temp_string[idx++] = name[i];
    }
    temp_string[idx+1] = 0;
  }
  return temp_string;
}

// requires TX_SYNTAX_VERBOSE_REGEX
// requires TX_SYNTAX_ASSIGN_NAME

void dump(TxNode *n, int level);

void dump_pattern(TxSyntax *syn, bool contained) {
  if (!syn) return;

  if (syn->rx_begin && !syn->rx_end) {
    // while not supported
    return;
  }

  // printf("----------\n");
  if (syn->rx_begin) {
    // return; // disable for now
    printf("\nsyn region");
  } else if (syn->rx_match) {
    printf("\nsyn match");
  } else {
    return;
  }

  char name[MAX_STRING_LENGTH];
  if (syn->content_name)
    strcpy(name, cleanup_name(syn->content_name));
  else if (syn->scope_name)
    strcpy(name, cleanup_name(syn->scope_name));
  else if (syn->name)
    strcpy(name, cleanup_name(syn->name));

  printf(" %s", name);

  if (syn->rx_begin) {
    printf(" start=%s end=%s", cleanup_regex(syn->rxs_begin), cleanup_regex(syn->rxs_end));
  } else if (syn->rx_match) {
    printf(" %s", cleanup_regex(syn->rxs_match));
  }

  if (syn->patterns) {
    printf(" contains=");
    TxNode *p = syn->patterns ? syn->patterns->first_child : NULL;
    bool did_contain = false;
    while(p) {
      TxSyntax *ps = txn_syntax_value_proxy(p);
      if (did_contain) {
        printf(",");
      }
      if (ps && ps->name) {
        printf("%s", cleanup_name(ps->name));
        did_contain = true;
      }
      p = p->next_sibling;
    }
  }

  if (contained) {
    printf(" contained");
  }
  
  printf("\n");

  if (strstr(name, "keyword")) {
    printf("hi link %s Keyword\n", name);
  }
  else if (strstr(name, "preprocessor")) {
    printf("hi link %s Preprocessor\n", name);
  }
  else if (strstr(name, "storage")) {
    printf("hi link %s Type\n", name);
  }
  else if (strstr(name, "variable")) {
    printf("hi link %s Variable\n", name);
  }
  else if (strstr(name, "constant")) {
    printf("hi link %s Constant\n", name);
  }
}

int main(int argc, char **argv) {

  printf("syn off\n");

  tx_initialize();

  char *default_path = "./samples/c.json";
  char *path = default_path;

  if (argc > 1) {
    path = argv[1];
  }

  TxNode *global_repository = tx_global_repository();
  TxSyntaxNode *root = txn_load_syntax(path);
  txn_set(global_repository, "source.c", root);

  TxSyntax *syn = txn_syntax_value(root);

    // printf("************\n");
    // printf(" patterns \n");
    // printf("************\n");

  // regions/matches (contains)
  TxNode *p = syn->patterns->first_child;
  while(p) {
    TxSyntax *ps = txn_syntax_value_proxy(p);
    dump_pattern(ps, false);
    p = p->next_sibling;
  }

    // printf("************\n");
    // printf(" repository \n");
    // printf("************\n");

  // region/matches (contained)
  p = syn->repository->first_child;
  while(p) {
    TxSyntax *ps = txn_syntax_value_proxy(p);
    dump_pattern(ps, true);
    p = p->next_sibling;
  }

  // dump(root, 0);

  // printf("grammar loaded at %fsecs\n", _cpu_time_used);

  tx_shutdown();
  // tx_stats();

  return 0;
}