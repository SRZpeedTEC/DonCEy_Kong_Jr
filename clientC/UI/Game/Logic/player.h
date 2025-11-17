#pragma once
// Player module: owns player state (position, size, velocity, flags).

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t x, y;      // top-left position in pixels
    int16_t w, h;      // size in pixels
    int16_t vx, vy;    // velocity in px/frame (temporary)
    bool grounded;     // true if standing on ground
    bool onVine;  // true if player is touching a vine

    int  vineSide;      // -1 = left of vine, +1 = right of vine
    bool vineSideLock;  // true after swapping side, until no horizontal input

    bool vineForcedFall;   // when active jr falls straight down 
    int16_t vineFallLockedX; // x position to keep during forced fall

    bool betweenVines;   // true when jr is holding two vines at once
    int  vineLeftIndex;  // index of left vine in st->vines
    int  vineRightIndex; // index of right vine in st->vines

    // jump state for constant-velocity model
    bool jumping;         // true while ascending
    int  jumpFramesLeft;  // remaining ascent frames
} Player;


// Initialize player with defaults.
void player_init(Player* p, int16_t x, int16_t y, int16_t w, int16_t h);

