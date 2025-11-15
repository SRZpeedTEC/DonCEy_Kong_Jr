// Player module implementation.
#include "player.h"

void player_init(Player* p, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!p) return;
    p->x = x; p->y = y; p->w = w; p->h = h;
    p->vx = 0; p->vy = 0;
    p->grounded = false;
    p->jumping = false;
    p->onVine = false;
    p->jumpFramesLeft = 0;

}