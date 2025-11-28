#include "collision.h"
#include "constants.h"
#include "fruit.h"
#include "../static_map.h"

// aabb overlap test between two rectangles
bool rects_overlap_i(int ax, int ay, int aw, int ah,
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

//small internal rect helper
typedef struct {
    int left, top, right, bottom, width, height;
} IntRect;


//build rect for current player position
static IntRect player_rect(const Player* p) {
    IntRect r;
    r.left   = p->x;
    r.top    = p->y;
    r.width  = p->w;
    r.height = p->h;
    r.right  = r.left + r.width;
    r.bottom = r.top  + r.height;
    return r;
}

//build rect for a static platform
static IntRect plat_rect(const CP_Rect* p) {
    IntRect r;
    r.left   = p->x;
    r.top    = p->y;
    r.width  = p->w;
    r.height = p->h;
    r.right  = r.left + r.width;
    r.bottom = r.top  + r.height;
    return r;
}


// build a smaller rect for vine detection (centered vertically inside player)
static IntRect player_vine_rect(const Player* p) {
    IntRect r;

    int fullLeft   = p->x;
    int fullTop    = p->y;
    int fullWidth  = p->w;
    int fullHeight = p->h;

    // use half of the height of Jr, size 
    int probeHeight = fullHeight / 2;
    if (probeHeight < 4) probeHeight = 4;  // small safety clamp

    // center the probe vertically inside the player hitbox
    int offsetY = (fullHeight - probeHeight) / 2;

    // small horizontal tolerance to make grabbing vines easier
    int extraX = 1;

    r.left   = fullLeft;
    r.top    = fullTop + offsetY;
    r.width  = fullWidth + 2 * extraX;
    r.height = probeHeight;
    r.right  = r.left + r.width;
    r.bottom = r.top  + r.height;

    return r;
}


// reach rect used when trying to grab a neighbor vine by stretching
// direction: -1 = reach left, +1 = reach right
static IntRect player_vine_reach_rect(const Player* p, int direction) {
    IntRect r;

    int fullLeft   = p->x;
    int fullTop    = p->y;
    int fullWidth  = p->w;
    int fullHeight = p->h;

    int probeHeight = fullHeight / 2;
    if (probeHeight < 4) probeHeight = 4;

    int offsetY = (fullHeight - probeHeight) / 2;

    // how far jr stretches beyond his body
    const int extraReach = 9; // PODEMOS CAMBIAR ESTO PARA VER EL AGARRE. <------------------------------

    if (direction > 0) {
        // reaching to the right
        r.left   = fullLeft;
        r.width  = fullWidth + extraReach;
    } else {
        // reaching to the left
        r.left   = fullLeft - extraReach;
        r.width  = fullWidth + extraReach;
    }

    r.top    = fullTop + offsetY;
    r.height = probeHeight;
    r.right  = r.left + r.width;
    r.bottom = r.top  + r.height;

    return r;
}

// check if player is vertically overlapping vines on both sides
bool player_between_vines(const Player* player,
                          const MapView* map)
{
    (void)map; // not needed now
    if (!player) return false;
    return player->betweenVines;
}

//handle vertical hit (top or bottom) against one platform
static bool vertical_hit(Player* player,
                         const IntRect* pr,
                         int prevTop,
                         int prevBottom,
                         const CP_Rect* platform)
{
    IntRect pl = plat_rect(platform);

    // ignore if no overlap
    if (!rects_overlap_i(pr->left, pr->top, pr->width, pr->height,
                         pl.left, pl.top, pl.width, pl.height))
        return false;

    bool falling = (player->vy > 0);
    bool rising  = (player->vy < 0);

    // need horizontal overlap for top/bottom hit
    bool horiz = (pr->right > pl.left && pr->left < pl.right);

    // falling onto top face
    if (falling &&
        horiz &&
        prevBottom <= pl.top &&
        pr->bottom >= pl.top)
    {
        player->y  = pl.top - player->h;
        player->vy = 0;
        return true;
    }

    // rising into bottom face
    if (rising &&
        horiz &&
        prevTop >= pl.bottom &&
        pr->top <= pl.bottom)
    {
        player->y  = pl.bottom;
        player->vy = 0;
        return true;
    }

    // if we reach here, overlap is lateral for vertical purposes
    return false;
}


// check if player overlaps any water rectangle
bool player_hits_water(const Player* player, const MapView* map) {
    (void)map; // no se usa más

    if (!player) return false;

    // Everything at y >= 226 is water
    if (player->y + player->h >= WATER_LINE_Y) {
        return true;
    }

    return false;
}

bool player_touching_mario(const Player* player) {
    if (!player) return false;

    return rects_overlap_i(player->x, player->y, player->w, player->h,
                           MARIO_X1, MARIO_Y1, MARIO_WIDTH, MARIO_HEIGHT);
}


//handle horizontal hit (left or right wall) against one platform
static bool horizontal_hit(Player* player,
                           const IntRect* pr,
                           int prevLeft,
                           int prevRight,
                           const CP_Rect* platform)
{
    IntRect pl = plat_rect(platform);

    // ignore if no overlap
    if (!rects_overlap_i(pr->left, pr->top, pr->width, pr->height,
                         pl.left, pl.top, pl.width, pl.height))
        return false;

    bool right = (player->vx > 0);
    bool left  = (player->vx < 0);

    // need vertical overlap for a side hit
    bool vert = (pr->bottom > pl.top && pr->top < pl.bottom);

    // moving right into left side
    if (right && vert &&
        prevRight <= pl.left &&
        pr->right >= pl.left)
    {
        player->x  = pl.left - player->w;
        player->vx = 0;
        return true;
    }

    // moving left into right side
    if (left && vert &&
        prevLeft >= pl.right &&
        pr->left <= pl.right)
    {
        player->x  = pl.right;
        player->vx = 0;
        return true;
    }

    return false;
}

//main entry for platform collisions (horizontal or vertical phase)
void resolve_player_platform_collisions(Player* player,
                                        const MapView* map,
                                        CollisionPhase phase)
{
    if (!player || !map || !map->data) return;

    const CP_Static* st = (const CP_Static*)map->data;
    if (!st->plat || st->nPlat == 0) return;

    IntRect pr = player_rect(player);

    if (phase == COLLISION_PHASE_VERTICAL) {

        int prevTop    = pr.top    - player->vy;
        int prevBottom = pr.bottom - player->vy;

        for (uint16_t i = 0; i < st->nPlat; i++) {
            if (vertical_hit(player, &pr, prevTop, prevBottom, &st->plat[i]))
                return;
        }

    } else { // horizontal phase

        int prevLeft  = pr.left  - player->vx;
        int prevRight = pr.right - player->vx;

        for (uint16_t i = 0; i < st->nPlat; i++) {
            if (horizontal_hit(player, &pr, prevLeft, prevRight, &st->plat[i]))
                return;
        }
    }
}

//recompute grounded based on floor or top of platforms
void update_player_grounded(Player* player,
                            const MapView* map,
                            int worldTop,
                            int worldHeight)
{
    if (!player || !map || !map->data) {
        player->grounded = false;
        return;
    }

    const CP_Static* st = (const CP_Static*)map->data;

    int floorY = worldTop + worldHeight - player->h;
    bool onFloor = (player->y == floorY);

    bool onPlat = false;
    if (st->plat && st->nPlat > 0) {

        IntRect pr = player_rect(player);

        for (uint16_t i = 0; i < st->nPlat; i++) {
            IntRect pl = plat_rect(&st->plat[i]);

            bool horiz   = (pr.right > pl.left && pr.left < pl.right);
            bool sameTop = (pr.bottom == pl.top);

            if (horiz && sameTop) {
                onPlat = true;
                break;
            }
        }
    }

    player->grounded = (onFloor || onPlat);
}

//VINES

//detect if player touches any vine
bool player_touching_vine(const Player* player, const MapView* map) {
    if (!player || !map || !map->data) return false;

    const CP_Static* st = (const CP_Static*)map->data;
    if (!st->vines || st->nVines == 0) return false;

    IntRect pr = player_vine_rect(player);

    for (uint16_t i = 0; i < st->nVines; i++) 
    {
        const CP_Rect* v = &st->vines[i];
        IntRect vr = plat_rect(v); 

        if (rects_overlap_i(pr.left, pr.top, pr.width, pr.height, vr.left, vr.top, vr.width, vr.height)) 
        {
            return true;
        }
    }

    return false;
}

// find which vine the player is currently holding onto.
// this checks the player's inner vine-rect against all vines.
// returns the index of the vine, or -1 if none.
int collision_find_current_vine_index(const Player* player,
                                      const MapView* map)
{
    if (!player || !map || !map->data)
        return -1;

    const CP_Static* st = (const CP_Static*)map->data;
    if (!st->vines || st->nVines == 0)
        return -1;

    // inner rect used to detect real vine grabbing
    IntRect pr = player_vine_rect(player);

    // check overlap with each vine
    for (uint16_t i = 0; i < st->nVines; i++) {
        IntRect vr = plat_rect(&st->vines[i]);

        if (rects_overlap_i(pr.left, pr.top, pr.width, pr.height,
                            vr.left, vr.top, vr.width, vr.height))
        {
            return (int)i; // player is grabbing this vine
        }
    }

    return -1; // no vine found
}

// find closest vine to the left or right that jr can reach when stretching.
// direction: -1 = left, +1 = right.
int collision_find_neighbor_vine_reachable(const Player* player,
                                           const MapView* map,
                                           int currentIndex,
                                           int direction)
{
    if (!player || !map || !map->data) return -1;
    if (direction == 0) return -1;

    const CP_Static* st = (const CP_Static*)map->data;
    if (!st->vines || st->nVines == 0) return -1;
    if (currentIndex < 0 || currentIndex >= (int)st->nVines) return -1;

    // rect of how far jr can stretch in this direction
    IntRect reach = player_vine_reach_rect(player, direction);

    const CP_Rect* cur = &st->vines[currentIndex];
    int curCenterX = cur->x + cur->w / 2;

    int bestIndex = -1;
    int bestDist  = 0;

    for (uint16_t i = 0; i < st->nVines; i++) {
        if ((int)i == currentIndex) continue;

        const CP_Rect* v = &st->vines[i];
        IntRect vr = plat_rect(v);

        // need some vertical overlap between reach window and vine
        bool vert = (reach.bottom > vr.top && reach.top < vr.bottom);
        if (!vert) continue;

        // vine must actually intersect the reach rect horizontally
        if (!rects_overlap_i(reach.left, reach.top, reach.width, reach.height,
                             vr.left, vr.top, vr.width, vr.height)) {
            continue;
        }

        int centerX = v->x + v->w / 2;
        int dx = centerX - curCenterX;

        // enforce correct side
        if (direction > 0 && dx <= 0) continue; // need vine to the right
        if (direction < 0 && dx >= 0) continue; // need vine to the left

        int dist = (dx > 0) ? dx : -dx;
        if (bestIndex < 0 || dist < bestDist) {
            bestIndex = (int)i;
            bestDist  = dist;
        }
    }

    return bestIndex;
}

void collision_update_between_vines_state(Player* player,
                                          const MapView* map)
{
    if (!player || !map || !map->data) return;
    if (!player->betweenVines) return;

    const CP_Static* st = (const CP_Static*)map->data;
    if (!st->vines || st->nVines == 0) {
        player->betweenVines = false;
        player->onVine       = false;
        return;
    }

    int li = player->vineLeftIndex;
    int ri = player->vineRightIndex;

    if (li < 0 || li >= (int)st->nVines ||
        ri < 0 || ri >= (int)st->nVines ||
        li == ri)
    {
        // bad pair, drop state
        player->betweenVines = false;
        player->onVine       = false;
        return;
    }

    const CP_Rect* lv = &st->vines[li];
    const CP_Rect* rv = &st->vines[ri];

    // "hands" y-position: middle of player
    int grabY = player->y + player->h / 2;

    bool onLeft =
        (grabY >= lv->y) && (grabY <= lv->y + lv->h);
    bool onRight =
        (grabY >= rv->y) && (grabY <= rv->y + rv->h);

    // still holding both vines
    if (onLeft && onRight) {
        player->onVine = true;
        return;
    }

    // only left vine still under Jr -> stay on left vine only
    if (onLeft && !onRight) {
        player->betweenVines = false;
        player->onVine       = true;

        int vineCenterX   = lv->x + lv->w / 2;
        int playerCenterX = player->x + player->w / 2;

        if (playerCenterX <= vineCenterX) {
            player->x = lv->x - (player->w - 1);
        } else {
            player->x = lv->x + lv->w - 1;
        }
        return;
    }

    // only right vine still under Jr -> stay on right vine only
    if (!onLeft && onRight) {
        player->betweenVines = false;
        player->onVine       = true;

        int vineCenterX   = rv->x + rv->w / 2;
        int playerCenterX = player->x + player->w / 2;

        if (playerCenterX <= vineCenterX) {
            player->x = rv->x - (player->w - 1);
        } else {
            player->x = rv->x + rv->w - 1;
        }
        return;
    }

    // no vine supports Jr at this height anymore
    player->betweenVines = false;
    player->onVine       = false;
}

// pick any overlapping fruit: deactivate it and mark event in player
bool player_pick_fruits(Player* player, Fruit* fruits, int count)
{
    if (!player || !fruits || count <= 0) return false;

    // reset event for this frame
    player->justPickedFruit = false;

    // build player rect
    IntRect pr = player_rect(player);

    for (int i = 0; i < count; ++i) {
        if (!fruits[i].active) continue;

        IntRect fr;
        fr.left   = fruits[i].x;
        fr.top    = fruits[i].y;
        fr.width  = fruits[i].w;
        fr.height = fruits[i].h;
        fr.right  = fr.left + fr.width;
        fr.bottom = fr.top  + fr.height;

        if (rects_overlap_i(pr.left, pr.top, pr.width, pr.height,
                            fr.left, fr.top, fr.width, fr.height))
        {
            fruits[i].active = false;
            player->justPickedFruit = true;
            player->lastPickedFruitX = fruits[i].x;
            player->lastPickedFruitY = fruits[i].y;
            return true; // one fruit per frame is suficiente
        }
    }

    return false;
}



