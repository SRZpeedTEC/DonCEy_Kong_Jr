#include "fruit.h"

void fruit_init(Fruit* fruit, int16_t defaultW, int16_t defaultH) {
    if (!fruit) return;
    fruit->active = false;
    fruit->x = 0;
    fruit->y = 0;
    fruit->w = defaultW;
    fruit->h = defaultH;
}

void fruit_spawn(Fruit* fruit, int16_t x, int16_t y) {
    if (!fruit) return;
    fruit->x = x;
    fruit->y = y;
    fruit->active = true;
}