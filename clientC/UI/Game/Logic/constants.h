#pragma once
// Basic tuning for movement and jump (pixels per frame)


#define MOVE_SPEED_X         1   // very slow horizontal move
#define FALL_SPEED_Y         1   // constant fall speed 
#define JUMP_ASCENT_SPEED   -1   // very slow upward motion
#define JUMP_ASCENT_FRAMES  18   // how many frames we keep ascending


// crocodile horizontal speed in pixels per second (base speed)
#define CROC_BASE_SPEED 1   

#define WATER_LINE_Y 226

// climbing speed when holding a single vine
#define VINE_CLIMB_SPEED_SLOW   1

// climbing speed when holding two vines (between vines)
#define VINE_CLIMB_SPEED_FAST   2

// Maximum number of crocodiles in the game
#define MAX_CROCS 16


// Maximun number of fruits in the game
#define MAX_FRUITS 16

// mario kill rectangle 
#define MARIO_X1 65
#define MARIO_X2 77
#define MARIO_Y1 49
#define MARIO_Y2 64

#define MARIO_WIDTH  (MARIO_X2 - MARIO_X1)
#define MARIO_HEIGHT (MARIO_Y2 - MARIO_Y1)