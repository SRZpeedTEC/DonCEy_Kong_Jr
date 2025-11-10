// clientC/core/tlv.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct { const uint8_t* p; uint32_t len; } TLVBuf;

static inline void tlv_init(TLVBuf* b, const uint8_t* p, uint32_t len){
  b->p = p; b->len = len;
}

static inline bool tlv_next(TLVBuf* b, uint8_t* type, uint16_t* L, const uint8_t** V){
  if (b->len < 3) return false;
  *type = b->p[0];
  *L    = (uint16_t)((b->p[1]<<8) | b->p[2]);
  if (b->len < (uint32_t)(3 + *L)) return false;
  *V = b->p + 3;
  b->p  += 3 + *L;
  b->len -= 3 + *L;
  return true;
}
