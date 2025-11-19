#pragma once
#include <stdint.h>

#define CP_VERSION 1

// Frames (capa externa)
enum {
  CP_TYPE_CLIENT_ACK   = 0x01, // s -> c
  CP_TYPE_INIT_GEOM    = 0x02, // s -> c (LEGACY INIT_STATIC)
  CP_TYPE_INIT_STATIC  = 0x02, // alias
  CP_TYPE_STATE_BUNDLE = 0x10, // s -> c (TLV adentro)
  CP_TYPE_TLV_STATE_HEADER = 0x11,
  CP_TYPE_TLV_PLAYER_CORR  = 0x12,
  CP_TYPE_TLV_ENTITIES_CORR = 0x13,
  CP_TYPE_PLAYER_PROP  = 0x20, // c -> s (inputs / propuesta)
  CP_TYPE_SPAWN_CROC   = 0x30, // s -> c (spawn crocodile at coords)
  CP_TYPE_SPAWN_FRUIT  = 0x40, // s -> c (spawn fruit at coords)
  CP_TYPE_REMOVE_FRUIT = 0x41,  // s -> c (remove fruit at coords)
  CP_TYPE_SPECTATOR_STATE = 0x50, // s -> spectator (player state update)

  CP_TYPE_NOTIFY_DEATH_COLLISION = 0x60, // s -> c (notify death by collision)
  CP_TYPE_NOTIFY_VICTORY       = 0x61,  // s -> c (notify victory)

  CP_TYPE_RESPAWN_DEATH_COLLISION = 0x70, // c -> s (request respawn after death by collision)
  CP_TYPE_GAME_OVER = 0x71,
  CP_TYPE_RESPAWN_WIN       = 0x72  // c -> s (request respawn after victory)
};

// TLVs (dentro de STATE_BUNDLE)
enum {
  TLV_STATE_HEADER  = 0x10, // u32 tick
  TLV_PLAYER_CORR   = 0x11, // u8 grounded, i16 platId, i16 yCorr, i16 vyCorr
  TLV_ENTITIES_CORR = 0x12  // futuro
};
