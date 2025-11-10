// proto.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t  version, type;
  uint16_t reserved;
  uint32_t clientId, gameId, payloadLen;
} CP_Header;

static inline uint16_t be16(const uint8_t *p){ return (uint16_t)((p[0]<<8)|p[1]); }
static inline uint32_t be32(const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3]; }
static inline void wr_be16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static inline void wr_be32(uint8_t *p, uint32_t v){ p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }

bool cp_read_header(int sock, CP_Header *h);
bool cp_write_header(int sock, const CP_Header *h);
bool cp_send_frame(int sock, uint8_t type, uint32_t clientId, uint32_t gameId,
                   const void* payload, uint32_t len);
