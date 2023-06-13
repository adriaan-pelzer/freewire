#include "stdlib.h"
#include "string.h"
#include "freewire_word.h"

fw_word_t *fw_word_create() {
  fw_word_t *ctx = (fw_word_t *) malloc(sizeof(fw_word_t));
  if (ctx) {
    memset(ctx, 0, sizeof(fw_word_t));
  }
  return ctx;
}

uint32_t fw_word_take_reading(fw_word_t *ctx, uint32_t reading) {
  uint32_t siglevel = (reading > 15) ? 1 : 0;
  uint32_t halfbit = (siglevel ? 31 : 15) - reading;

  if (ctx->stale) {
    memset(ctx, 0, sizeof(fw_word_t));
  }

  if ((ctx->halfbit_index & 63) == 0) {
    ctx->value = 0;
  }

  if (reading == 0xffffffff) {
    if (ctx->halfbit_index < 3) {
      ctx->stale = 1;
      return 0;
    }
    halfbit = ctx->period - ctx->prev;
  }

  if (ctx->halfbit_index & 1) {
    if (halfbit < ctx->prev) {
      ctx->value |= (1 << (31 - ((ctx->halfbit_index & 63) >> 1)));
    }
  } else {
    ctx->prev = halfbit;
  }

  if (ctx->halfbit_index < 2) {
    ctx->period += halfbit;
  }

  ctx->halfbit_index++;

  if ((ctx->halfbit_index & 63) == 0) {
    return 1;
  }

  if (reading == 0xffffffff) {
    ctx->stale = 1;
    return 1;
  }

  return 0;
}

uint32_t fw_word_get_value(fw_word_t *ctx) {
  return ctx->value;
}

void fw_word_destroy(fw_word_t *ctx) {
  if (ctx) {
    free(ctx);
  }
}
