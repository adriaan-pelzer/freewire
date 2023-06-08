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
#include "freewire_word.h"

#define NUM_WORDS 100

void print_binary(uint32_t value) {
  for (uint32_t i = 0; i < 32; i++) {
    printf("%u", (value >> (31 - i)) & 1);
  }
}

uint32_t values[NUM_WORDS];
uint32_t value_index = 0;

void core_one(void) {
  PIO pio = (PIO) multicore_fifo_pop_blocking();
  int sm1 = (int) multicore_fifo_pop_blocking();
  fw_word_t fw_word;

  memset(&fw_word, 0, sizeof(fw_word_t));

  printf("pio [core 1]: %p\n", pio);
  printf("sm1 [core 1]: %i\n", sm1);

  uint offset = pio_add_program(pio, &freewire_read_program);

  printf("Loaded program at %d [core 1]\n", offset);

  freewire_read_program_init(pio, sm1, offset, 5);

  while (true) {
    if (fw_word_take_reading(&fw_word, pio_sm_get_blocking(pio, sm1))) {
      values[value_index] = fw_word.value;
      if (value_index++ == (NUM_WORDS - 1)) {
        value_index = 0;
        memset(&fw_word, 0, sizeof(fw_word_t));
      }
    }
  }
}

uint32_t _random() {
  return (uint32_t)((rand() << 1) | (rand() & 1));
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
    if (1) {
      uint32_t num[NUM_WORDS];

      for (uint32_t i = 0; i < NUM_WORDS; i++) {
        num[i] = _random();
        pio_sm_put_blocking(pio, sm0, num[i]);
      }

      sleep_ms(20);
      for (uint32_t i = 0; i < NUM_WORDS; i++) {
        if (num[i] == values[i]) {
          printf(".");
        } else {
          printf("X");
        }
        if (count++ % 80 == 79) {
          printf("\n");
        }
      }
    }
  }
}
