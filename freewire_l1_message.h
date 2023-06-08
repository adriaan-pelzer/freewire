#ifndef _FREEWIRE_MESSAGE_L1_H_
#define _FREEWIRE_MESSAGE_L1_H_

#define FW_L1_MESSAGE_MAX_LENGTH 256

#include "pico/stdlib.h"

typedef enum {
  FW_L1_MESSAGE_ERR_OK,
  FW_L1_MESSAGE_ERR_ZEROLEN,
  FW_L1_MESSAGE_ERR_MAXLEN,
  FW_L1_MESSAGE_ERR_CHKSUM,
  FW_L1_MESSAGE_ERR_MORE
} fw_l1_message_err_t;

typedef struct {
  uint32_t length;
  uint32_t body[FW_L1_MESSAGE_MAX_LENGTH];
  uint32_t body_index;
  uint32_t checksum;
} fw_l1_message_t;

fw_l1_message_t *fw_l1_new_message();
fw_l1_message_err_t fw_l1_fill_message(fw_l1_message_t *message, uint32_t *body, uint32_t length);
fw_l1_message_err_t fw_l1_consume_word(fw_l1_message_t *message, uint32_t word);
void fw_l1_clear_message(fw_l1_message_t *message);
void fw_l1_destroy_message(fw_l1_message_t *message);

#endif // _FREEWIRE_L1_H_
