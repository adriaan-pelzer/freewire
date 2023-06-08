#ifndef _FREEWIRE_WORD_H_
#define _FREEWIRE_WORD_H_

#include "pico/stdlib.h"

typedef struct {
  uint32_t value;
  uint32_t period;
  uint32_t halfbit_index;
  uint32_t prev;
  uint32_t stale;
} fw_word_t;

uint32_t fw_word_take_reading(fw_word_t *ctx, uint32_t reading);

#endif // _FREEWIRE_WORD_H_
