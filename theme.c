#include "textmate.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static struct {
  char *name;
  char *value;
} scope_remap[] = {{"function", "entity.name.function"},
                   // { "tag", "entity.name.tag" },
                   {"attribute", "entity.other.attribute-name"},
                   {"parameter", "variable.parameter"},
                   {"argument", "variable.parameter"},
                   {"preprocessor.", "meta.preprocessor"},
                   // { "punctuation", "constant.character.escape" },
                   // { "foreground", "text.source" },
                   // {"block", "foreground"},
                   {"type.", "entity.name.type"},
                   {NULL, NULL}};

bool _tx_style_from_scope(char *scope, TxTheme *theme, TxStyleSpan *style) {
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
}

bool tx_style_from_scope(char *scope, TxTheme *theme, TxStyleSpan *style) {
  if (_tx_style_from_scope(scope, theme, style)) {
    return true;
  }

  TxNode *token_colors = theme->token_colors;

  // quick remap
  if (theme && scope) {
    for (int i = 0;; i++) {
      if (!scope_remap[i].name)
        break;
      if (strstr(scope, scope_remap[i].name)) {
        if (_tx_style_from_scope(scope_remap[i].value, theme, style)) {
          return true;
        }
        // printf("[%s] > %s\n", scope, scope_remap[i].name);
      }
    }
    // txn_push(theme->unresolved_scopes, txn_new_string(scope));
  }

  return false;
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

bool txt_parse_color(const char *color, uint32_t *result) {
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
  char *ptr;
  uint32_t parsed = strtoul(color, &ptr, 16);
  if (*ptr != '\0') {
    return false;
  }
  *result = len == 6 ? ((parsed << 8) | 0xFF) : parsed;
  return true;
}