#include "textmate.h"

void line_start(TxParseProcessor *self, char_u *buffer_start,
                char_u *buffer_end) {}

void line_end(TxParseProcessor *self) {}

void open_tag(TxParseProcessor *self, TxMatch *state) {}

void close_tag(TxParseProcessor *self, TxMatch *state) {}

void capture(TxParseProcessor *self, TxMatch *state) {}

void tx_init_processor(TxParseProcessor *processor) {
  memset(processor, 0, sizeof(TxParseProcessor));
}
