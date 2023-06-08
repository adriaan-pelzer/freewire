#include "stdlib.h"
#include "string.h"
#include "freewire_l1_message.h"

fw_l1_message_t *fw_l1_new_message() {
  fw_l1_message_t *message = (fw_l1_message_t *) malloc(sizeof(fw_l1_message_t));
  if (message) {
    memset(message, 0, sizeof(fw_l1_message_t));
  }
  return message;
}

static uint32_t checksum(fw_l1_message_t *message) {
  uint32_t checksum = 0;
  for (uint32_t i = 0; i < message->length; i++) {
    checksum += message->body[i];
  }
  return checksum;
}

static fw_l1_message_err_t allocate_body(fw_l1_message_t *message, uint32_t length) {
  if (message->body != NULL) {
    return FW_L1_MESSAGE_ERR_USED;
  }

  if ((message->body = (uint32_t *) malloc(length * sizeof(uint32_t))) == NULL) {
    return FW_L1_MESSAGE_ERR_MALLOC;
  }

  message->length = length;

  return FW_L1_MESSAGE_ERR_OK;
}

fw_l1_message_err_t fw_l1_fill_message(fw_l1_message_t *message, uint32_t *body, uint32_t length) {
  fw_l1_message_err_t rc = FW_L1_MESSAGE_ERR_OK;

  if ((rc = allocate_body(message, length)) != FW_L1_MESSAGE_ERR_OK) {
    return rc;
  }

  memcpy(message->body, body, length * sizeof(uint32_t));
  message->checksum = checksum(message);

  return rc;
}

fw_l1_message_err_t fw_l1_consume_word(fw_l1_message_t *message, uint32_t word) {
  if (message->body == NULL) {
    fw_l1_message_err_t rc = FW_L1_MESSAGE_ERR_OK;
    if (word == 0) {
      return FW_L1_MESSAGE_ERR_EMPTY;
    }
    if ((rc = allocate_body(message, word)) != FW_L1_MESSAGE_ERR_OK) {
      return rc;
    }
    return FW_L1_MESSAGE_ERR_MORE;
  }

  if (message->body_index == message->length) {
    if (checksum(message) != word) {
      return FW_L1_MESSAGE_ERR_CHKSUM;
    }
    message->checksum = word;
    return FW_L1_MESSAGE_ERR_OK;
  }

  message->body[message->body_index++] = word;

  if (message->body_index == message->length) {
    return FW_L1_MESSAGE_ERR_MORE;
  }

  return FW_L1_MESSAGE_ERR_MORE;
}

void fw_l1_clear_message(fw_l1_message_t *message) {
  if (message->body) {
    free(message->body);
  }
  memset(message, 0, sizeof(fw_l1_message_t));
}

void fw_l1_destroy_message(fw_l1_message_t *message) {
  if (message) {
    fw_l1_clear_message(message);
    free(message);
  }
}
