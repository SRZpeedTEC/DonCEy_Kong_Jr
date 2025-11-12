#pragma once
#include "../game/static_map.h"

// Simple render module for drawing the level and the player box.
// Receives static map and player bounds.


struct CP_Static;

void render_draw_level(const struct CP_Static* s,
                       int playerX, int playerY, int playerW, int playerH);



