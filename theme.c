#include "textmate.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool tx_style_from_scope(char_u *scope, TxTheme *theme, TxStyleSpan *style) {
  memset(style, 0, sizeof(TxStyleSpan));

  bool found = false;
  TxNode *token_colors = theme->token_colors;
  TxNode *child = token_colors->first_child;
  while (child) {
    if (child->name) {
      if (strstr(scope, child->name) != NULL) {
        TxFontStyle *fsv = txn_font_style_value(child);
        memcpy(&style->font_style, fsv, sizeof(TxFontStyle));
        found = true;
        break;
      }
    }
    child = child->next_sibling;
  }

  return found;

  // slow but more tolerant resolution (but currently inaccurate)
  if (!found && scope) {
    char temp[TX_SCOPE_NAME_LENGTH];

    txn_push(theme->unresolved_scopes, txn_new_string(scope));

    TxNode *match = NULL;
    int match_score = 0;
    int failure = 0;
    TxNode *child = token_colors->first_child;
    while (child) {
      if (child->name) {
        char_u *anchor = child->name;
        int score = 0;

        strcpy(temp, scope);
        char_u *token = strtok(temp, ".");
        while (token) {
          char_u *pos = strstr(anchor, token);
          if (pos) {
            anchor = pos;
            score++;
          }
          token = strtok(NULL, ".");
        }

        if (score > 0) {
          if (score > match_score) {
            // printf("\t%s %d\n", child->name, score);
            match = child;
            match_score = score;
          }
        }
      }
      child = child->next_sibling;
    }

    if (match) {
      TxFontStyle *fsv = txn_font_style_value(match);
      memcpy(&style->font_style, fsv, sizeof(TxFontStyle));
      found = true;

      // add an entry so that we won't have to tokenize again
      TxFontStyleNode *fs = txn_new_font_style();
      TxFontStyle *_fsv = txn_font_style_value(fs);
      memcpy(_fsv, fsv, sizeof(TxFontStyle));
      txn_set(token_colors, scope, fs);

      // printf("%s > %s\n", scope, match->name);
    }
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