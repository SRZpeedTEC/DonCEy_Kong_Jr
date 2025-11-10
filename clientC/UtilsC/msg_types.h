#pragma once
#include <stdint.h>

#define CP_VERSION 1

// Frames (capa externa)
enum {
  CP_TYPE_CLIENT_ACK   = 0x01, // s -> c
  CP_TYPE_INIT_GEOM    = 0x02, // s -> c (LEGACY INIT_STATIC)
  CP_TYPE_INIT_STATIC  = 0x02, // alias
  CP_TYPE_STATE_BUNDLE = 0x10, // s -> c (TLV adentro)
  CP_TYPE_PLAYER_PROP  = 0x20, // c -> s (inputs / propuesta)
  CP_TYPE_PING         = 0x7E,
  CP_TYPE_PONG         = 0x7F
};

// TLVs (dentro de STATE_BUNDLE)
enum {
  TLV_STATE_HEADER  = 0x10, // u32 tick
  TLV_PLAYER_CORR   = 0x11, // u8 grounded, i16 platId, i16 yCorr, i16 vyCorr
  TLV_ENTITIES_CORR = 0x12  // futuro
};
