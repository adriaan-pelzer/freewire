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

uint32_t shared_value = 0;

typedef struct {
  uint32_t active;
  uint32_t bins[32];
  uint32_t values[32];
  uint32_t bin_index;
  uint32_t prev;
  uint32_t period;
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

void count_bits(count_t *counter, uint32_t bit) {
  if (counter->period > 0) {
    if (counter->bins[counter->bin_index] == counter->period / 4) {
      counter->values[counter->bin_index] = bit;
    }
    if (counter->bins[counter->bin_index] == counter->period / 2) {
      counter->bin_index++;
      if (counter->bin_index == 32) {
        shared_value = convert_count(counter);
        memset(counter, 0, sizeof(count_t));
      }
    }
    counter->bins[counter->bin_index]++;
    return;
  }
  if (counter->active == 0) {
    if (bit == 0) {
      counter->bins[0]++;
    } else {
      if (counter->bins[0] > 1024) {
        counter->active = 1;
        counter->bins[0] = 1;
      }
    }
  } else {
    if (bit != counter->prev) {
      counter->values[counter->bin_index] = counter->prev;
      counter->bin_index++;
      if (counter->bin_index == 2) {
        counter->period = counter->bins[0] + counter->bins[1];
      }
    }
    counter->bins[counter->bin_index]++;
  }
  counter->prev = bit;
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
    for (uint32_t i = 0; i < 32; i++) {
      count_bits(&counter, pop_bit(&value));
    }
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
  sleep_ms(2000);
  multicore_launch_core1(core_one);

  multicore_fifo_push_blocking((uint32_t) pio);
  multicore_fifo_push_blocking((uint32_t) sm1);

  freewire_write_program_init(pio, sm0, offset, 2);

  while (true) {
    uint32_t num = (uint32_t) rand();
    num = num | (1 << 31);
    num = num | (1 << 29);
    num = num & 0xbfffffff;
    pio_sm_put_blocking(pio, sm0, num);
    pio_sm_put_blocking(pio, sm0, 0x00000000);
    sleep_ms(1000);
    if (num != shared_value) {
      printf("\n *** mismatch: %u != %u\n", num, shared_value);
    } else {
      printf(".");
      if (count++ == 32) {
        printf("\n");
        count = 0;
      }
    }
  }
}
