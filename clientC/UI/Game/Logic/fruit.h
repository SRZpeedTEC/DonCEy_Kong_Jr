#pragma once

// Fruits state and helpers.

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool active;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} Fruit;

// Initialize fruit with default size and inactive state.
void fruit_init(Fruit* fruit, int16_t defaultW, int16_t defaultH);

// Activate fruit at given position.
void fruit_spawn(Fruit* fruit, int16_t x, int16_t y);

