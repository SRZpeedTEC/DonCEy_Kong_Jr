// physics.c — constant fall speed, slow jump ascent, special vine movement
#include "physics.h"
#include "constants.h"
#include "collision.h"
#include "static_map.h"

// simple toggle to slow vine climbing 
static int g_vineClimbToggle = 0;

// Map left/right input to horizontal velocity.
static void updateHorizontalVelocityFromInput(Player* player, const InputState* input) {
    int direction = 0; // -1 left, 0 idle, +1 right
    if (input->left)  direction -= 1;
    if (input->right) direction += 1;
    player->vx = (int16_t)(direction * MOVE_SPEED_X);
}

// Move player on X using current horizontal velocity.
static void applyHorizontalMotion(Player* player) {
    player->x += player->vx;
}

// Decide vertical velocity using a constant-velocity model when not on a vine.
// If grounded and jump is requested then start slow ascent for N frames.
// While ascending, keep constant upward speed until frames run out.
// Otherwise constant fall speed.
static void updateVerticalVelocityConstant(Player* player, const InputState* input) {
    if (player->grounded) {
        if (input->jump) {
            player->jumping = true;
            player->jumpFramesLeft = JUMP_ASCENT_FRAMES;
            player->vy = (int16_t)JUMP_ASCENT_SPEED; // slow upward
            player->grounded = false;
        } else {
            player->jumping = false;
            player->jumpFramesLeft = 0;
            player->vy = 0; // stay on floor
        }
        return;
    }

    if (player->jumping && player->jumpFramesLeft > 0) {
        player->vy = (int16_t)JUMP_ASCENT_SPEED; // keep going up slowly
        player->jumpFramesLeft--;
        if (player->jumpFramesLeft == 0) {
            player->jumping = false; // switch to falling next frame
        }
    } else {
        player->vy = (int16_t)FALL_SPEED_Y; // constant fall
    }
}

// Move player on Y using current vertical velocity.
static void applyVerticalMotion(Player* player) {
    player->y += player->vy;
}

// Vertical movement while on a vine: no gravity, only up/down.
// if betweenVines is true, jr climbs faster.
static void updateVerticalVelocityOnVine(Player* player,
                                         const InputState* input,
                                         bool betweenVines)
{
    int speed = betweenVines ? VINE_CLIMB_SPEED_FAST
                             : VINE_CLIMB_SPEED_SLOW;

    if (input->up) {
        player->vy = (int16_t)(-speed);
    } else if (input->down) {
        player->vy = (int16_t)( speed);
    } else {
        player->vy = 0;
    }
}

// Keep player inside a rectangular world (left/top/right/bottom).
static void clampInsideWorldBounds(Player* player,
                                   int worldLeft, int worldTop,
                                   int worldWidth, int worldHeight)
{
    // walls
    int rightLimit = worldLeft + worldWidth - player->w;
    if (player->x < worldLeft)  player->x = worldLeft;
    if (player->x > rightLimit) player->x = rightLimit;

    // ceiling
    if (player->y < worldTop) {
        player->y = worldTop;
        if (player->vy < 0) player->vy = 0;
        player->jumping = false;
        player->jumpFramesLeft = 0;
    }

    // floor
    int floorY = worldTop + worldHeight - player->h;
    if (player->y > floorY) {
        player->y = floorY;
        if (player->vy > 0) player->vy = 0;
        player->jumping = false;
        player->jumpFramesLeft = 0;
    }
}

// Grounded = true iff player stands exactly on the floor (no platforms yet).
static void updateGroundedFromFloor(Player* player, int worldTop, int worldHeight) {
    int floorY = worldTop + worldHeight - player->h;
    player->grounded = (player->y == floorY);
    if (player->grounded) {
        player->jumping = false;
        player->jumpFramesLeft = 0;
    }
}

// handle horizontal input while jr is on a vine.
// left/right can either swap side on the same vine or try to reach a neighbor.
static bool handleVineHorizontal(Player* player,
                                 const InputState* input,
                                 const MapView* map)
{
    if (!player || !input || !map || !map->data)
        return false;

    // no horizontal input: clear lock and do nothing
    if (!input->left && !input->right) {
        player->vineSideLock = false;
        return false;
    }

    const CP_Static* st = (const CP_Static*)map->data;

    bool between = player_between_vines(player, map);

    // direction from input: -1 left, +1 right
    int dir = 0;
    if (input->left)  dir = -1;
    if (input->right) dir = +1;
    if (dir == 0) return false;

    // case: already between two vines, choose one side
    if (between) {
        int leftIndex  = player->vineLeftIndex;
        int rightIndex = player->vineRightIndex;

        // pick right vine
        if (dir > 0 && rightIndex >= 0 && rightIndex < (int)st->nVines) {
            const CP_Rect* rv = &st->vines[rightIndex];
            player->x = rv->x + rv->w - 1;
            player->betweenVines = false;
            player->vineSideLock = true;
            return false;
        }

        // pick left vine
        if (dir < 0 && leftIndex >= 0 && leftIndex < (int)st->nVines) {
            const CP_Rect* lv = &st->vines[leftIndex];
            player->x = lv->x - (player->w - 1);
            player->betweenVines = false;
            player->vineSideLock = true;
            return false;
        }

        return false;
    }

    // case: holding a single vine
    int currentIndex = collision_find_current_vine_index(player, map);
    if (currentIndex < 0) {
        player->onVine          = false;
        player->betweenVines    = false;
        player->vineLeftIndex   = -1;
        player->vineRightIndex  = -1;
        player->vineForcedFall  = true;
        player->vineFallLockedX = player->x;
        player->vineSideLock    = false;
        return true;
    }

    const CP_Rect* v = &st->vines[currentIndex];

    int vineCenterX   = v->x + v->w / 2;
    int playerCenterX = player->x + player->w / 2;

    // true if player is visually to the right of this vine
    bool onRightSide = (playerCenterX > vineCenterX);

    // "towards" the vine center vs "away"
    bool towards = (dir < 0 && onRightSide) || (dir > 0 && !onRightSide);
    bool away    = !towards;

    // pressing towards the vine -> swap side on same vine
    if (towards) {
        if (!player->vineSideLock) {
            if (onRightSide) {
                // was on right, go to left
                player->x = v->x - (player->w - 1);
            } else {
                // was on left, go to right
                player->x = v->x + v->w - 1;
            }
            player->vineSideLock = true;
        }
        return false;
    }

    // pressing away from the vine
    if (away) {
        // try to reach a neighbor vine in this direction
        int neighborIndex = collision_find_neighbor_vine_reachable(player, map,
                                                                   currentIndex, dir);
        if (neighborIndex >= 0) {
            // found neighbor: jr stretches and ends up between both vines
            const CP_Rect* nv = &st->vines[neighborIndex];

            int centerA = v->x  + v->w  / 2;
            int centerB = nv->x + nv->w / 2;
            int midX    = (centerA + centerB) / 2;

            player->x = midX - player->w / 2;

            // store which vine is left and which is right
            if (centerA <= centerB) {
                player->vineLeftIndex  = currentIndex;
                player->vineRightIndex = neighborIndex;
            } else {
                player->vineLeftIndex  = neighborIndex;
                player->vineRightIndex = currentIndex;
            }

            player->betweenVines = true;
            player->onVine       = true;
            player->vineSideLock = true; // one stretch per key press
            return false;
        }

        // no neighbor vine: require second press to drop
        if (player->vineSideLock) {
            // same key still held, ignore
            return false;
        }

        // second press away with no neighbor: drop straight down
        if (dir > 0) {
            player->x += 2;
        } else {
            player->x -= 2;
        }

        player->onVine          = false;
        player->betweenVines    = false;
        player->vineLeftIndex   = -1;
        player->vineRightIndex  = -1;
        player->vineForcedFall  = true;
        player->vineFallLockedX = player->x;
        player->vineSideLock    = true;
        return true;
    }

    return false;
}

// update vine state after movement.
// this handles forced drops, entering a vine and snapping to a side.
static void updateVineStateAfterMovement(Player* player,
                                         const MapView* map,
                                         bool wasOnVine,
                                         bool forcedDrop)
{
    if (!player || !map) return;

    // if we are in forced fall, never grab vines
    if (player->vineForcedFall) {
        player->onVine = false;
        return;
    }

    // if we are between two vines, keep that state here
    if (player->betweenVines) {
        player->onVine = true;
        return;
    }

    if (forcedDrop) {
        player->onVine       = false;
        player->betweenVines = false;
        player->vineLeftIndex  = -1;
        player->vineRightIndex = -1;
        return;
    }

    // normal case: recompute onVine from geometry
    bool nowOnVine = player_touching_vine(player, map);

    // if we just grabbed a vine this frame, snap to left or right side
    if (!wasOnVine && nowOnVine) {
        int idx = collision_find_current_vine_index(player, map);
        if (idx >= 0 && map->data) {
            const CP_Static* st = (const CP_Static*)map->data;
            const CP_Rect* v   = &st->vines[idx];

            int vineCenterX   = v->x + v->w / 2;
            int playerCenterX = player->x + player->w / 2;

            int side;
            if (player->vx > 0) {
                side = -1; // came from left
            } else if (player->vx < 0) {
                side = +1; // came from right
            } else {
                side = (playerCenterX <= vineCenterX) ? -1 : +1;
            }

            if (side < 0) {
                player->x = v->x - (player->w - 1);
            } else {
                player->x = v->x + v->w - 1;
            }

            player->vy             = 0;
            player->jumping        = false;
            player->jumpFramesLeft = 0;
            player->grounded       = false;

            // single-vine state on grab
            player->betweenVines   = false;
            player->vineLeftIndex  = -1;
            player->vineRightIndex = -1;

            player->vineSideLock = true;
        }
    }

    player->onVine = nowOnVine;
}
    




// Main function that makes one physics step with constant vertical speeds
// and special vine movement when onVine is true.
void physics_step(Player* player, const InputState* input, const MapView* map, float dt) {
    (void)dt; // per-frame model 
    if (!player || !input || !map) return;

    int worldLeft, worldTop, worldWidth, worldHeight;
    map_get_world_bounds(&worldLeft, &worldTop, &worldWidth, &worldHeight);

    bool forcedDrop = false;
    bool wasOnVine  = player->onVine;

    // horizontal phase
        if (player->vineForcedFall) {
            // lock x while falling from a vine
            player->vx = 0;
            player->x  = player->vineFallLockedX;
        } else if (player->onVine) {
            forcedDrop = handleVineHorizontal(player, input, map);
            player->vx = 0;
        } else {
            updateHorizontalVelocityFromInput(player, input);
            applyHorizontalMotion(player);
            resolve_player_platform_collisions(player, map, COLLISION_PHASE_HORIZONTAL);
        }

    // vertical phase
    if (player->onVine && !forcedDrop && !player->vineForcedFall) {
        bool betweenVines = player_between_vines(player, map);
        updateVerticalVelocityOnVine(player, input, betweenVines);
        applyVerticalMotion(player);
    } else {
        // forced fall or normal mode both use gravity model
        updateVerticalVelocityConstant(player, input);
        applyVerticalMotion(player);
    }

    // keep inside world bounds
    clampInsideWorldBounds(player, worldLeft, worldTop, worldWidth, worldHeight);

    // vertical collisions with platforms (top and bottom only)
    resolve_player_platform_collisions(player, map, COLLISION_PHASE_VERTICAL);

    // final grounded state (floor or platform top)
    update_player_grounded(player, map, worldTop, worldHeight);

    
    // vine state update
    // update between-vines state (only if not forced fall)
    if (!player->vineForcedFall) {
        collision_update_between_vines_state(player, map);
    }

    // vine state update (enter/leave single vines, snapping)
    updateVineStateAfterMovement(player, map, wasOnVine, forcedDrop);

    // stop forced fall when jr is grounded again
    if (player->vineForcedFall && player->grounded) {
        player->vineForcedFall = false;
    }

}