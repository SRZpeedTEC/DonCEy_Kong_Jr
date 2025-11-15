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

// handle left/right input while on a vine:
// - pressing "towards" vine swaps side around the same vine
// - pressing "away" from vine tries to reach a neighbor; if none, drops
static bool handleVineHorizontal(Player* player,
                                 const InputState* input,
                                 const MapView* map)
{
    if (!input->left && !input->right) return false; // no horizontal intent

    int currentIndex = collision_find_current_vine_index(player, map);
    if (currentIndex < 0) {
        // no vine actually under reach -> treat as drop
        player->onVine = false;
        return true; // dropped
    }

    // get side: compare player center vs vine center
    const CP_Static* st = (const CP_Static*)map->data;
    const CP_Rect* v = &st->vines[currentIndex];
    int vineCenterX   = v->x + v->w / 2;
    int playerCenterX = player->x + player->w / 2;
    int side = 0; // -1 = player mostly left, +1 = mostly right
    if (playerCenterX < vineCenterX) side = -1;
    else if (playerCenterX > vineCenterX) side = 1;
    else side = 1; // arbitrary if perfectly centered

    // if pressing opposite to side, we just swap around the same vine
    if (input->left && side > 0) {
        // was on right, go to left side of same vine
        player->x = v->x - player->w;
        return false;
    }
    if (input->right && side < 0) {
        // was on left, go to right side of same vine
        player->x = v->x + v->w;
        return false;
    }

    // pressing "away" from current side: try to grab a neighbor vine
    int dir = 0;
    if (input->left)  dir = -1;
    if (input->right) dir = +1;
    if (dir == 0) return false;

    int neighborIndex = collision_find_neighbor_vine_reachable(player, map,
                                                               currentIndex, dir);
    if (neighborIndex < 0) {
        // no neighbor vine in reach: jr drops
        player->onVine = false;
        return true; // dropped
    }

    // snap to neighbor vine side (right or left depending on direction)
    const CP_Rect* nv = &st->vines[neighborIndex];
    if (dir > 0) {
        // grabbing a vine to the right
        player->x = nv->x + nv->w;
    } else {
        // grabbing a vine to the left
        player->x = nv->x - player->w;
    }

    return false; // still on some vine
}




// Main function that makes one physics step with constant vertical speeds
// and special vine movement when onVine is true.
void physics_step(Player* player, const InputState* input, const MapView* map, float dt) {
    (void)dt; // per-frame model 
    if (!player || !input || !map) return;

    int worldLeft, worldTop, worldWidth, worldHeight;
    map_get_world_bounds(&worldLeft, &worldTop, &worldWidth, &worldHeight);

     bool forcedDrop = false;

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

    // detect vine contact for next frame (uses inner vine rect)
    // update onVine only if we did not force a drop this frame
    if (!forcedDrop) {
        player->onVine = player_touching_vine(player, map);
    }
}