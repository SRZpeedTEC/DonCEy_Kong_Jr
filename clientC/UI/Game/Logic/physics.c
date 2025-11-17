// physics.c — constant fall speed, slow jump ascent, special vine movement
#include "physics.h"
#include "constants.h"
#include "collision.h"
#include "static_map.h"

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
static void updateVerticalVelocityOnVine(Player* player, const InputState* input) {
    // disable jump state when on a vine
    player->jumping = false;
    player->jumpFramesLeft = 0;
    player->grounded = false;

    if (input->up && !input->down) {
        player->vy = (int16_t)(-VINE_CLIMB_SPEED); // climb up
    } else if (input->down && !input->up) {
        player->vy = (int16_t)(VINE_CLIMB_SPEED);  // climb down
    } else {
        player->vy = 0; // stay in place on the vine
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

// handle horizontal input while on a vine.
// left/right can either swap side on the same vine or try to reach a neighbor.
// vineSideLock makes sure a single key press does not do both.
static bool handleVineHorizontal(Player* player,
                                 const InputState* input,
                                 const MapView* map)
{
    if (!player || !input || !map || !map->data)
        return false;

    // no horizontal input: release lock and do nothing
    if (!input->left && !input->right) {
        player->vineSideLock = false;
        return false;
    }

    int currentIndex = collision_find_current_vine_index(player, map);
    if (currentIndex < 0) {
        // lost the vine for some reason: drop
        player->onVine       = false;
        player->vineSideLock = false;
        return true;
    }

    const CP_Static* st = (const CP_Static*)map->data;
    const CP_Rect* v    = &st->vines[currentIndex];

    int vineCenterX   = v->x + v->w / 2;
    int playerCenterX = player->x + player->w / 2;

    // true if player is visually to the right of the vine
    bool onRightSide = (playerCenterX > vineCenterX);

    // current input direction: -1 = left, +1 = right
    int dir = 0;
    if (input->left)  dir = -1;
    if (input->right) dir = +1;
    if (dir == 0) return false;

    // "towards" means moving closer to the vine center
    bool towards = (dir < 0 && onRightSide) || (dir > 0 && !onRightSide);
    bool away    = !towards;

    // step 1: pressing towards the vine -> swap side on this vine
    if (towards) {
        if (!player->vineSideLock) {
            // swap to opposite side on the same vine, with 1px overlap
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

    // step 2: pressing away from the vine
    // this should only act if the player already released the key after swapping
    if (away) {
        if (player->vineSideLock) {
            // still same key hold after swap -> ignore
            return false;
        }

        // try to reach a neighbor vine in this direction
        int neighborIndex = collision_find_neighbor_vine_reachable(player, map,
                                                                   currentIndex, dir);
        if (neighborIndex < 0) {
            // no neighbor vine: drop and move a bit away to avoid instant re-grab
            if (dir > 0) {
                player->x += 2;
            } else {
                player->x -= 2;
            }
            player->onVine       = false;
            player->vineSideLock = false;
            return true; // forced drop
        }

        // grab neighbor vine on the side of movement, with small overlap
        const CP_Rect* nv = &st->vines[neighborIndex];
        if (dir > 0) {
            // grabbing a vine to the right
            player->x = nv->x + nv->w - 1;
        } else {
            // grabbing a vine to the left
            player->x = nv->x - (player->w - 1);
        }

        player->vineSideLock = true; // need to release before next change
        return false;
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

    // if we forced a drop this frame, clear vine state and exit
    if (forcedDrop) {
        player->onVine       = false;
        player->vineSide     = 0;
        player->vineSideLock = false;
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

        // decide entry side mainly from horizontal velocity
        int side = 0; // -1 = left side of vine, +1 = right side
        if (player->vx > 0) {
            // jr was moving right -> came from left -> attach on left side
            side = -1;
        } else if (player->vx < 0) {
            // jr was moving left -> came from right -> attach on right side
            side = +1;
        } else {
            // no clear horizontal movement, fallback to center comparison
            int vineCenterX   = v->x + v->w / 2;
            int playerCenterX = player->x + player->w / 2;
            side = (playerCenterX <= vineCenterX) ? -1 : +1;
        }

        if (side < 0) {
            // snap to left side, overlapping 1 pixel into the vine
            player->x = v->x - (player->w - 1);
            player->vineSide = -1;
        } else {
            // snap to right side, overlapping 1 pixel into the vine
            player->x = v->x + v->w - 1;
            player->vineSide = +1;
        }

        // being on a vine cancels jump/fall
        player->vy = 0;
        player->jumping = false;
        player->jumpFramesLeft = 0;
        player->grounded = false;

        // fresh state for horizontal logic on the vine
        player->vineSideLock = false;
    }
    player->onVine = nowOnVine;
}
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
    if (player->onVine) {
        // horizontal input is interpreted as vine transitions
        forcedDrop = handleVineHorizontal(player, input, map);
        player->vx = 0;
    } else {
        updateHorizontalVelocityFromInput(player, input);
        applyHorizontalMotion(player);
        // horizontal collisions with platforms
        resolve_player_platform_collisions(player, map, COLLISION_PHASE_HORIZONTAL);
    }

    // vertical phase
    if (player->onVine && !forcedDrop) {
        // vine movement: only up/down, no gravity
        updateVerticalVelocityOnVine(player, input);
        applyVerticalMotion(player);
    } else {
        // normal jump/fall model
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
    updateVineStateAfterMovement(player, map, wasOnVine, forcedDrop);

}