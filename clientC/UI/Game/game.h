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
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity);



// build a TLV_ENTITIES_CORR into dst and return its size; 0 on error
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity);
void game_apply_remote_state(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t flags); //Spectator

void game_spawn_croc(uint8_t variant, int16_t x, int16_t y);

void game_spawn_fruit(uint8_t variant, int16_t x, int16_t y);

void game_remove_fruit_at(int16_t x, int16_t y);

void game_respawn_player(void);

void game_respawn_death(void);

void game_respawn_win(void);

bool game_consume_death_event(void);

bool game_consume_win_event(void);

bool game_consume_fruit_event(int16_t* outX, int16_t* outY);

void game_over_event(void);

void game_set_ui_lives(uint8_t lives);

void game_set_ui_score(uint32_t score);

// Increase global crocodile speed.
void crocodile_increase_speed(void);

static void game_check_win_condition(void);

void game_update_spectator(const CP_Static* staticMap);

void game_restart(void);
bool game_check_restart_clicked(void);

// Clear all dynamic entities (for spectator snapshot updates)
void game_clear_all_entities(void);