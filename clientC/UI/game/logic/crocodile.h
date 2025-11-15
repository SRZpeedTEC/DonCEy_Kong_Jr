#pragma once
// Simple crocodile state and helpers.

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool active;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Crocodile;

// Initialize crocodile with default size and inactive state.
void crocodile_init(Crocodile* croc, int16_t defaultW, int16_t defaultH);

// Activate crocodile at given position.
void crocodile_spawn(Crocodile* croc, int16_t x, int16_t y);

//function for movement and behavior. FALTA IMPLEMENTAR 
void crocodile_update(Crocodile* croc, float dt);