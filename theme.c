#include "textmate.h"

void txt_style_from_scope(char_u *scope, TxTheme *theme, TxStyleSpan *style) {
  TxNode *token_colors = theme->token_colors;
  TxNode *child = token_colors->first_child;
  while (child) {
    if (child->name) {
      if (strstr(child->name, scope) != -1) {
        style->fg = child->string_value;
      }
    }
    child = child->next_sibling;
  }
}