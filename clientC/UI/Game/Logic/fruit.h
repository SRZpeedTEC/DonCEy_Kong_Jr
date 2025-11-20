#pragma once

// Fruits state and helpers.

#include <stdint.h>
#include <stdbool.h>
#include "player.h"

#define FRUIT_VARIANT_BANANA 1
#define FRUIT_VARIANT_APPLE 2
#define FRUIT_VARIANT_ORANGE 3

typedef struct {
    bool active;
    uint8_t variant;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Fruit;


// Initialize fruit with default size and inactive state.
void fruit_init(Fruit* fruit, int16_t defaultW, int16_t defaultH);

// Activate fruit at given position.
void fruit_spawn(Fruit* fruit, uint8_t variant, int16_t x, int16_t y);

