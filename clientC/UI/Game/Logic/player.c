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
    p->isDead = false;
    p->justPickedFruit = false;

}

// simple AABB overlap helper used for fruit pickup
static bool aabb_overlap_int(int ax, int ay, int aw, int ah,
                             int bx, int by, int bw, int bh)
{
    int aRight  = ax + aw;
    int aBottom = ay + ah;
    int bRight  = bx + bw;
    int bBottom = by + bh;

    if (aRight  <= bx)      return false;
    if (ax      >= bRight)  return false;
    if (aBottom <= by)      return false;
    if (ay      >= bBottom) return false;
    return true;
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

