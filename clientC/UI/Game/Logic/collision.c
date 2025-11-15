#include "collision.h"
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

    r.left   = fullLeft;
    r.top    = fullTop + offsetY;
    r.width  = fullWidth;
    r.height = probeHeight;
    r.right  = r.left + r.width;
    r.bottom = r.top  + r.height;

    return r;
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

    for (uint16_t i = 0; i < st->nVines; i++) {
        const CP_Rect* v = &st->vines[i];
        IntRect vr = plat_rect(v); 

        if (rects_overlap_i(pr.left, pr.top, pr.width, pr.height,
                            vr.left, vr.top, vr.width, vr.height))
        {
            return true;
        }
    }

    return false;
}