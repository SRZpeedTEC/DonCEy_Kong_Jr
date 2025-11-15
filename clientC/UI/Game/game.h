#pragma once
#include <stdint.h>
#include "static_map.h"

typedef struct { int16_t x,y,vx,vy; uint8_t flags; } ProposedState;

void game_init(uint16_t vw, uint16_t vh, uint16_t scale);
void game_shutdown(void);
void game_set_bg(const char* path, float alpha);
void game_draw_static(const CP_Static* s);

void game_update_and_get_proposal(const CP_Static* s, ProposedState* out);
void game_apply_correction(uint32_t tick, uint8_t grounded, int16_t platId, int16_t yCorr, int16_t vyCorr);

void game_spawn_croc(int16_t x, int16_t y);

void game_spawn_fruit(int16_t x, int16_t y);
