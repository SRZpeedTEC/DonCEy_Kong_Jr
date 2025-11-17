#pragma once
// Basic tuning for movement and jump (pixels per frame)


#define MOVE_SPEED_X         1   // very slow horizontal move
#define FALL_SPEED_Y         1   // constant fall speed 
#define JUMP_ASCENT_SPEED   -1   // very slow upward motion
#define JUMP_ASCENT_FRAMES  18   // how many frames we keep ascending

// climbing speed when holding a single vine
#define VINE_CLIMB_SPEED_SLOW   1

// climbing speed when holding two vines (between vines)
#define VINE_CLIMB_SPEED_FAST   2

// Maximum number of crocodiles in the game
#define MAX_CROCS 16


// Maximun number of fruits in the game
#define MAX_FRUITS 16