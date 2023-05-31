/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "freewire_write.pio.h"
#include "freewire_read.pio.h"

uint32_t shared_value[2] = { 0, 0 };
uint32_t shared_value_idx = 0;

typedef enum {
  STATE_START = 0,
  STATE_INIT,
  STATE_RECV
} state_t;

typedef struct {
  state_t state;
  uint32_t active;
  uint32_t bins[32];
  uint32_t values[32];
  uint32_t bin_index;
  uint32_t prev;
  uint32_t half_period;
  uint32_t qtr_period;
} count_t;

uint32_t convert_count(count_t *counter) {
  uint32_t ret = 0;
  for (uint32_t i = 0; i < 32; i++) {
    ret = ret | (counter->values[i] << (31 - i));
  }
  return ret;
}

uint32_t pop_bit(uint32_t *value) {
  uint32_t ret = ((*value) >> 31) & 1;
  *value = ((*value) << 1) & 0xfffffffe;
  return ret;
}

void set_period(count_t *counter, uint32_t period) {
  counter->half_period = period / 2;
  counter->half_period = counter->half_period == 0 ? 1 : counter->half_period;
  counter->qtr_period = period / 4;
  counter->qtr_period = counter->qtr_period == 0 ? 1 : counter->qtr_period;
}

void process_value(count_t *counter, uint32_t value) {
  for (uint32_t i = 0; i < 32; i++) {
    uint32_t bit = pop_bit(&value);

    if (counter->state == STATE_RECV) {
      if (counter->bins[counter->bin_index] == counter->qtr_period) {
        counter->values[counter->bin_index] = bit;
      }
      if (counter->bins[counter->bin_index] == counter->half_period) {
        counter->bin_index++;
        if (counter->bin_index == 32) {
          shared_value[shared_value_idx] = convert_count(counter);
          memset(counter, 0, sizeof(count_t));
        }
      }
    } else {
      if (counter->state == STATE_START) {
        if (bit == 1 && counter->bins[0] > 1024) {
          shared_value_idx = 0;
          counter->state = STATE_INIT;
          counter->bins[0] = 1;
        }
      } else if (bit != counter->prev) {
        counter->values[counter->bin_index] = counter->prev;
        counter->bin_index++;
        if (counter->bin_index == 2) {
          counter->state = STATE_RECV;
          set_period(counter, counter->bins[0] + counter->bins[1]);
        }
      }
    }

    counter->bins[counter->bin_index]++;
    counter->prev = bit;
  }
}

void core_one(void) {
  PIO pio = (PIO) multicore_fifo_pop_blocking();
  int sm1 = (int) multicore_fifo_pop_blocking();
  count_t counter;

  memset(&counter, 0, sizeof(count_t));

  printf("pio [core 1]: %p\n", pio);
  printf("sm1 [core 1]: %i\n", sm1);

  uint offset = pio_add_program(pio, &freewire_read_program);

  printf("Loaded program at %d [core 1]\n", offset);

  freewire_read_program_init(pio, sm1, offset, 5);

  while (true) {
    uint32_t value = pio_sm_get_blocking(pio, sm1);
    process_value(&counter, value);
  }
}

int main() {
  stdio_init_all();
  PIO pio = pio0;
  int sm0 = pio_claim_unused_sm(pio, true);
  int sm1 = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &freewire_write_program);
  uint32_t count = 0;

  printf("pio [core 0]: %p\n", pio);
  printf("sm1 [core 0]: %i\n", sm1);

  printf("Loaded program at %d [core 0]\n", offset);

  multicore_reset_core1();
  sleep_ms(100);
  multicore_launch_core1(core_one);

  multicore_fifo_push_blocking((uint32_t) pio);
  multicore_fifo_push_blocking((uint32_t) sm1);

  freewire_write_program_init(pio, sm0, offset, 2);

  sleep_ms(100);

  while (true) {
    uint32_t num = (uint32_t) rand();

    num = num | (1 << 31);
    num = num | (1 << 29);
    num = num & 0xbfffffff;
    pio_sm_put_blocking(pio, sm0, num);
    pio_sm_put_blocking(pio, sm0, 0x00000000);
    sleep_ms(5);
    if (num != shared_value[0]) {
      printf("\n *** num mismatch: %u != %u\n", num, shared_value[0]);
    } else {
      printf(".");
      if (count++ == 32) {
        printf("\n");
        count = 0;
      }
    }
  }
}
