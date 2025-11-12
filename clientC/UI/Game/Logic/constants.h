#pragma once
// Basic tuning for movement and jump (pixels per frame)

#define MOVE_SPEED_X   2     // horizontal speed
#define GRAVITY_Y      1     // gravity applied every frame
#define JUMP_SPEED_Y  -12    // initial jump velocity (negative goes up)
#define MAX_FALL_Y     10    // clamp max fall velocity