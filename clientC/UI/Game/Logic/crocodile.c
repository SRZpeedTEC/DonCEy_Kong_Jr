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

// simple aabb helper inside this file
static bool rects_overlap_int(int ax, int ay, int aw, int ah,
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

// check if player overlaps any active crocodile
bool crocodile_player_overlap(const Player* player,
                              const Crocodile* crocs,
                              int count)
{
    if (!player || !crocs) return false;

    for (int i = 0; i < count; ++i) {
        if (!crocs[i].active) continue;

        if (rects_overlap_int(player->x, player->y, player->w, player->h,
                            crocs[i].x, crocs[i].y, crocs[i].w, crocs[i].h))
        {
            return true;
        }
    }

    return false;
}