#include "fruit.h"

void fruit_init(Fruit* fruit, int16_t defaultW, int16_t defaultH) {
    if (!fruit) return;
    fruit->active = false;
    fruit->variant = FRUIT_VARIANT_BANANA; // default variant
    fruit->x = 0;
    fruit->y = 0;
    fruit->w = defaultW;
    fruit->h = defaultH;
}

void fruit_spawn(Fruit* fruit, uint8_t variant, int16_t x, int16_t y) {
    if (!fruit) return;
    fruit->variant = variant;
    fruit->x = x;
    fruit->y = y;
    fruit->active = true;
}