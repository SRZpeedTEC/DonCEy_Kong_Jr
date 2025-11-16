#include "crocodile.h"

void crocodile_init(Crocodile* croc, int16_t defaultW, int16_t defaultH) {
    if (!croc) return;
    croc->active = false;
    croc->variant = CROC_VARIANT_RED; // default variant
    croc->x = 0;
    croc->y = 0;
    croc->w = defaultW;
    croc->h = defaultH;
}

void crocodile_spawn(Crocodile* croc, uint8_t variant, int16_t x, int16_t y) {
    if (!croc) return;
    croc->variant = variant;
    croc->x = x;
    croc->y = y;
    croc->active = true;
}

void crocodile_update(Crocodile* croc, float dt) {
    (void)croc;
    (void)dt;
    // Movement logic will be added later.
}