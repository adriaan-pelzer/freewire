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
#include "freewire_l1_message.h"

#define NUM_WORDS 100

fw_l1_message_t *message = NULL;

void core_one(void) {
  PIO pio = (PIO) multicore_fifo_pop_blocking();
  int sm1 = (int) multicore_fifo_pop_blocking();
  uint offset = pio_add_program(pio, &freewire_read_program);
  fw_word_t *fw_word = NULL;

  if ((fw_word = fw_word_create()) == NULL) {
    printf("cannot create word context\n");
    return;
  }

  if ((message = fw_l1_new_message()) == NULL) {
    printf("cannot create new message\n");
    return;
  }

  printf("\npio [core 1]: %p\n", pio);
  printf("sm1 [core 1]: %i\n", sm1);
  printf("Loaded program at %d [core 1]\n", offset);

  freewire_read_program_init(pio, sm1, offset, 5);

  while (true) {
    if (fw_word_take_reading(fw_word, pio_sm_get_blocking(pio, sm1))) {
      fw_l1_message_err_t rc = FW_L1_MESSAGE_ERR_OK;
      if ((rc = fw_l1_consume_word(message, fw_word_get_value(fw_word))) != FW_L1_MESSAGE_ERR_MORE) {
        if (rc != FW_L1_MESSAGE_ERR_OK) {
          printf("cannot consume word: %u\n", rc);
        }
      }
    }
  }

  fw_word_destroy(fw_word);
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

  printf("\npio [core 0]: %p\n", pio);
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
      fw_l1_message_t *sent_message = fw_l1_new_message();
      uint32_t num[NUM_WORDS];

      for (uint32_t i = 0; i < NUM_WORDS; i++) {
        num[i] = _random();
      }

      if (fw_l1_fill_message(sent_message, num, NUM_WORDS) != FW_L1_MESSAGE_ERR_OK) {
        printf("cannot fill message\n");
        continue;
      }

      pio_sm_put_blocking(pio, sm0, sent_message->length);
      for (uint32_t i = 0; i < sent_message->length; i++) {
        pio_sm_put_blocking(pio, sm0, sent_message->body[i]);
      }
      pio_sm_put_blocking(pio, sm0, sent_message->checksum);

      sleep_ms(10);

      if (sent_message->length != message->length) {
        printf("L");
      } else {
        printf(".");
      }

      count++;

      for (uint32_t i = 0; i < NUM_WORDS; i++) {
        if (sent_message->body[i] == message->body[i]) {
          printf(".");
        } else {
          printf("x");
        }
        if (count++ % 80 == 79) {
          printf("\n");
        }
      }

      if (sent_message->checksum != message->checksum) {
        printf("C");
      } else {
        printf(".");
      }

      count++;

      fw_l1_clear_message(message);
      fw_l1_destroy_message(sent_message);
    }
  }
}
