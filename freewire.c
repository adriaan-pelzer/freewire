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

uint32_t shared_value;

typedef enum {
  STATE_START = 0,
  STATE_RUN
} state_t;

typedef struct {
  state_t state;
  uint32_t prev;
  uint32_t ones[32];
  uint32_t zeros[32];
  uint32_t index;
} count_t;

uint32_t pop_bit(uint32_t *value) {
  uint32_t ret = ((*value) >> 31) & 1;
  *value = ((*value) << 1) & 0xfffffffe;
  return ret;
}

void display_counter(count_t *counter) {
  for (uint32_t i = 0; i < 32; i++) {
    printf("%u: %u %u\n", i, counter->ones[i], counter->zeros[i]);
  }
}

uint32_t counter_value(count_t *counter) {
  uint32_t ret = 0;
  for (uint32_t i = 0; i < 32; i++) {
    if (counter->ones[i] > counter->zeros[i]) {
      ret |= (1 << (31 - i));
    }
  }
  return ret;
}

void process_value(count_t *counter, uint32_t value) {
  for (uint32_t i = 0; i < 32; i++) {
    uint32_t bit = pop_bit(&value);

    switch (counter->state) {
      case STATE_START:
        if (bit == 0) {
          counter->zeros[0]++;
        } else {
          if (counter->zeros[0] > 1024) {
            counter->state = STATE_RUN;
            counter->index = 0;
            counter->ones[counter->index] = 1;
          } else {
            counter->zeros[0] = 0;
          }
        }
        break;
      case STATE_RUN:
        if (bit != counter->prev) {
          if (bit == 1) {
            counter->index++;
            if (counter->index == 32) {
              counter->index = 0;
            }
            counter->ones[counter->index] = 0;
          } else {
            counter->zeros[counter->index] = 0;
          }
        }
        if (bit == 1) {
          counter->ones[counter->index]++;
        } else {
          counter->zeros[counter->index]++;
          if (counter->zeros[counter->index] > 1024) {
            counter->zeros[counter->index] = (counter->zeros[0] + counter->ones[0]) - counter->ones[counter->index];
            counter->state = STATE_START;
            shared_value = counter_value(counter);
            //display_counter(counter);
            counter->zeros[0] = 0;
            counter->index = 0;
          }
        }
        break;
    }

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
    process_value(&counter, pio_sm_get_blocking(pio, sm1));
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
    sleep_ms(8);
    if (num != shared_value) {
      printf("!!! %u != %u\n", num, shared_value);
    } else {
      printf(".");
    }
    count++;
    if (count == 32) {
      printf("\n");
      count = 0;
    }
  }
}
