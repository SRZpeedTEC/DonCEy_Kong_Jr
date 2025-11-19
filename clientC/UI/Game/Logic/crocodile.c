#include "crocodile.h"
#include <stddef.h>
#include "../static_map.h"
#include "map.h"
#include "constants.h"

// number of frames between crocodile movement updates
#define CROC_FRAME_DIV 3

// internal base speed shared by all crocodiles
static int g_crocBaseSpeed = 0;

// global frame counter to slow down crocodile updates
static int g_crocFrameCounter = 0;

// compute the current speed in pixels per movement step
static int croc_speed(void) {
    if (g_crocBaseSpeed <= 0) {
        g_crocBaseSpeed = (CROC_BASE_SPEED > 0) ? CROC_BASE_SPEED : 1;
    }

    int s = g_crocBaseSpeed;
    if (s < 1) s = 1;
    return s;
}

// external hook to increase crocodile speed for higher difficulty
void crocodile_increase_speed(void) {
    if (g_crocBaseSpeed <= 0) {
        g_crocBaseSpeed = (CROC_BASE_SPEED > 0) ? CROC_BASE_SPEED : 1;
    }
    g_crocBaseSpeed++;
}

// simple aabb helper
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

// helper to access static map from view
static const CP_Static* map_static(const MapView* map) {
    if (!map) return NULL;
    return (const CP_Static*)map->data;
}

// check if croc is standing on top of a platform
static bool croc_on_platform(const Crocodile* c,
                             const CP_Static* st,
                             int* outIndex)
{
    if (!c || !st || !st->plat || st->nPlat == 0) return false;

    int feetY = c->y + c->h;
    int left  = c->x;
    int right = c->x + c->w;

    for (uint16_t i = 0; i < st->nPlat; ++i) {
        const CP_Rect* p = &st->plat[i];

        bool horiz = (right > p->x) && (left < p->x + p->w);
        bool onTop = (feetY == p->y);

        if (horiz && onTop) {
            if (outIndex) *outIndex = (int)i;
            return true;
        }
    }
    return false;
}

// check if croc center is inside a vine rectangle
static bool croc_on_vine(const Crocodile* c,
                         const CP_Static* st,
                         int* outIndex)
{
    if (!c || !st || !st->vines || st->nVines == 0) return false;

    int cx = c->x + c->w / 2;
    int cy = c->y + c->h / 2;

    for (uint16_t i = 0; i < st->nVines; ++i) {
        const CP_Rect* v = &st->vines[i];

        bool insideX = (cx >= v->x) && (cx < v->x + v->w);
        bool insideY = (cy >= v->y) && (cy < v->y + v->h);

        if (insideX && insideY) {
            if (outIndex) *outIndex = (int)i;
            return true;
        }
    }
    return false;
}

// blue crocodile: slide down vines, walk on platforms, fall into water
static void update_blue_croc(Crocodile* c,
                             const CP_Static* st,
                             int worldTop,
                             int worldHeight)
{
    int speed     = croc_speed();
    int platIndex = -1;
    int vineIndex = -1;

    bool onVine = croc_on_vine(c, st, &vineIndex);
    bool onPlat = croc_on_platform(c, st, &platIndex);

    if (onVine) {
        c->vx = 0;
        c->vy = speed;
        c->y += c->vy;
        return;
    }

    if (onPlat) {
        const CP_Rect* p = &st->plat[platIndex];

        // start moving right when first landing on a platform
        if (c->vx == 0) {
            c->vx = speed;
        }

        c->x += c->vx;

        // still overlapping this platform?
        bool stillOn =
            (c->x + c->w > p->x) &&
            (c->x < p->x + p->w);

        // if we left the platform, start falling straight down
        if (!stillOn) {
            c->vx = 0;
            c->vy = speed;
            c->y += c->vy;
        }
        return;
    }

    // default: fall straight down
    c->vx = 0;
    c->vy = speed;
    c->y += c->vy;

    int worldBottom = worldTop + worldHeight;
    if (c->y > worldBottom + 16) {
        c->active = false;
    }
}

// red crocodile: slide down vines, then oscillate on platforms
static void update_red_croc(Crocodile* c,
                            const CP_Static* st,
                            int worldTop,
                            int worldHeight)
{
    int speed     = croc_speed();
    int platIndex = -1;
    int vineIndex = -1;

    bool onVine = croc_on_vine(c, st, &vineIndex);
    bool onPlat = croc_on_platform(c, st, &platIndex);

    if (onVine && !onPlat) {
        c->vx = 0;
        c->vy = speed;
        c->y += c->vy;
        return;
    }

    if (onPlat) {
        const CP_Rect* p = &st->plat[platIndex];

        // if no direction yet, start to the right
        if (c->vx == 0) {
            c->vx = speed;
        }

        c->x += c->vx;

        int leftLimit  = p->x;
        int rightLimit = p->x + p->w - c->w;

        // bounce at platform edges
        if (c->x < leftLimit) {
            c->x  = leftLimit;
            c->vx = speed;      // go right
        } else if (c->x > rightLimit) {
            c->x  = rightLimit;
            c->vx = -speed;     // go left
        }

        c->vy = 0;
        return;
    }

    // in air: fall until we remove the crocodile
    c->vx = 0;
    c->vy = speed;
    c->y += c->vy;

    int worldBottom = worldTop + worldHeight;
    if (c->y > worldBottom + 16) {
        c->active = false;
    }
}

// initialize crocodile with default size and inactive state
void crocodile_init(Crocodile* croc, int16_t defaultW, int16_t defaultH) {
    if (!croc) return;
    croc->active  = false;
    croc->variant = CROC_VARIANT_RED; // default variant
    croc->x = 0;
    croc->y = 0;
    croc->w = defaultW;
    croc->h = defaultH;
    croc->vx = 0;
    croc->vy = 0;
}

// activate crocodile at given position
void crocodile_spawn(Crocodile* croc, uint8_t variant, int16_t x, int16_t y) {
    if (!croc) return;
    croc->variant = variant;
    croc->x = x;
    croc->y = y;
    croc->vx = 0;
    croc->vy = 0;
    croc->active = true;
}

// per-frame update for a single crocodile
void crocodile_update(Crocodile* croc, const MapView* map) {
    if (!croc || !croc->active || !map) return;

    // slow down updates so crocodiles move only every N frames
    g_crocFrameCounter++;
    if (g_crocFrameCounter % CROC_FRAME_DIV != 0) {
        return;
    }

    const CP_Static* st = map_static(map);
    if (!st) return;

    int worldLeft, worldTop, worldWidth, worldHeight;
    map_get_world_bounds(&worldLeft, &worldTop, &worldWidth, &worldHeight);

    // type-specific movement
    if (croc->variant == CROC_VARIANT_BLUE) {
        update_blue_croc(croc, st, worldTop, worldHeight);
    } else {
        update_red_croc(croc, st, worldTop, worldHeight);
    }

    // clamp to world borders horizontally and flip direction if we hit edges
    int maxX = worldLeft + worldWidth - croc->w;

    if (croc->x <= worldLeft) {
        croc->x = worldLeft;
        if (croc->vx < 0) {
            croc->vx = croc_speed(); // bounce to the right
        }
    } else if (croc->x >= maxX) {
        croc->x = maxX;
        if (croc->vx > 0) {
            croc->vx = -croc_speed(); // bounce to the left
        }
    }
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