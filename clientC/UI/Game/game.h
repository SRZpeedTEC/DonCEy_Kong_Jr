#pragma once
#include <stdint.h>
#include <stddef.h>   
#include "static_map.h"

typedef struct { int16_t x,y,vx,vy; uint8_t flags; } ProposedState;

void game_init(uint16_t vw, uint16_t vh, uint16_t scale);
void game_shutdown(void);
void game_set_bg(const char* path, float alpha);
void game_draw_static(const CP_Static* s);

void game_update_and_get_proposal(const CP_Static* s, ProposedState* out);
void game_apply_correction(uint32_t tick, uint8_t grounded, int16_t platId, int16_t yCorr, int16_t vyCorr);

// build a TLV_ENTITIES_CORR into dst and return its size; 0 on error
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity);

void game_spawn_croc(uint8_t variant, int16_t x, int16_t y);

void game_spawn_fruit(uint8_t variant, int16_t x, int16_t y);

void game_remove_fruit_at(int16_t x, int16_t y);

void game_respawn_player(void);
