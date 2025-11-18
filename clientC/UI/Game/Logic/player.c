// Player module implementation.
#include "player.h"


// reset all vine / jump / death flags for a fresh life
void player_init(Player* p, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!p) return;
    p->x = x; p->y = y; p->w = w; p->h = h;
    p->vx = 0; p->vy = 0;
    p->grounded = false;
    p->jumping = false;
    p->onVine = false;
    p->jumpFramesLeft = 0;
    

}




// mark player as dead once
void player_mark_dead(Player* p) {
    if (!p) return;
    if (p->isDead) return; // already dead

    p->isDead = true;
    p->justDied = true;

    p->vx = 0;
    p->vy = 0;

    p->onVine = false;
    p->betweenVines = false;
    p->vineForcedFall = false;
    p->vineSideLock = false;
}

// helper checks
bool player_is_dead(const Player* p) {
    return p && p->isDead;
}

bool player_just_died(Player* p) {
    if (!p) return false;
    if (!p->justDied) return false;
    p->justDied = false; // consume the event
    return true;
}