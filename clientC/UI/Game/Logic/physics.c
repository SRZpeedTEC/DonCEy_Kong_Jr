// physics.c — constant fall speed, slow jump ascent, special vine movement
#include "physics.h"
#include "constants.h"
#include "collision.h"

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

// Main function that makes one physics step with constant vertical speeds
// and special vine movement when onVine is true.
void physics_step(Player* player, const InputState* input, const MapView* map, float dt) {
    (void)dt; // per-frame model for now
    if (!player || !input || !map) return;

    int worldLeft, worldTop, worldWidth, worldHeight;
    map_get_world_bounds(&worldLeft, &worldTop, &worldWidth, &worldHeight);

    // horizontal phase
    if (player->onVine) {
        // no horizontal movement while on a vine (CHANGE LATER)
        player->vx = 0;
    } else {
        updateHorizontalVelocityFromInput(player, input);
        applyHorizontalMotion(player);
        // resolve side hits with platforms
        resolve_player_platform_collisions(player, map, COLLISION_PHASE_HORIZONTAL);
    }

    // vertical phase
    if (player->onVine) {
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
    player->onVine = player_touching_vine(player, map);
}