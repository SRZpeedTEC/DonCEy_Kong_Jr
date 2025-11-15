#pragma once
// Basic tuning for movement and jump (pixels per frame)


#define MOVE_SPEED_X         1   // very slow horizontal move
#define FALL_SPEED_Y         1   // constant fall speed 
#define JUMP_ASCENT_SPEED   -1   // very slow upward motion
#define JUMP_ASCENT_FRAMES  18   // how many frames we keep ascending

#define VINE_CLIMB_SPEED 1 // speed used when climbing vines (pixels per frame)


// Maximum number of crocodiles in the game
#define MAX_CROCS 16


// Maximun number of fruits in the game
#define MAX_FRUITS 16