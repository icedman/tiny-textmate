#include "textmate.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

bool tx_style_from_scope(char_u *scope, TxTheme *theme, TxStyleSpan *style) {
  memset(style, 0, sizeof(TxStyleSpan));

  bool found = false;
  TxNode *token_colors = theme->token_colors;
  TxNode *child = token_colors->first_child;
  while (child) {
    if (child->name) {
      if (strstr(scope, child->name) != NULL) {
        TxFontStyle *fsv = txn_font_style_value(child);
        style->fg = fsv->fg;
        style->bg = fsv->bg;
        style->italic = fsv->italic;
        style->bold = fsv->bold;
        style->underline = fsv->underline;
        found = true;
        // break;
      }
    }
    child = child->next_sibling;
  }

  if (!found) {
    // printf("!%s\n", scope);
    // if (strcmp(scope, "meta.qualified_type.cpp") == 0) {
    // style->fg = txt_make_color(255,0,255);
    // style->bold = true;
    // found = true;
    // }
  }

  return found;
}

bool txt_color_to_rgb(uint32_t color, uint32_t result[3]) {
  result[0] = ((color >> 24) & 0xff);
  result[1] = ((color >> 16) & 0xff);
  result[2] = ((color >> 8) & 0xff);
  return true;
}

uint32_t txt_make_color(int r, int g, int b) {
  return (r << 24) | (g << 16) | (b << 8) | 0xff;
}

bool txt_parse_color(const char_u *color, uint32_t *result) {
  if (!color) {
    return 0xffffff;
  }
  if (color[0] == '#') {
    ++color;
  }
  int len = strlen(color);
  if ((len != 6 && len != 8) || !isxdigit(color[0]) || !isxdigit(color[1])) {
    return false;
  }
  char_u *ptr;
  uint32_t parsed = strtoul(color, &ptr, 16);
  if (*ptr != '\0') {
    return false;
  }
  *result = len == 6 ? ((parsed << 8) | 0xFF) : parsed;
  return true;
}