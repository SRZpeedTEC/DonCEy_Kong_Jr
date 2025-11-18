#pragma once
// Simple crocodile state and helpers.

#include <stdint.h>
#include <stdbool.h>
#include "player.h"

#define CROC_VARIANT_RED 1
#define CROC_VARIANT_BLUE 2

typedef struct {
    bool active;
    uint8_t variant;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Crocodile;

// Initialize crocodile with default size and inactive state.
void crocodile_init(Crocodile* croc, int16_t defaultW, int16_t defaultH);

// Activate crocodile at given position.
void crocodile_spawn(Crocodile* croc, uint8_t variant, int16_t x, int16_t y);

//function for movement and behavior. FALTA IMPLEMENTAR 
void crocodile_update(Crocodile* croc, float dt);

bool crocodile_player_overlap(const Player* player, const Crocodile* crocs, int count);