#pragma once
// Simple physics: constant fall speed and slow jump ascent.

#include "player.h"
#include "input.h"
#include "map.h"

// Runs one physics step: input → velocities → motion → clamp → grounded.
void physics_step(Player* player, const InputState* input, const MapView* map, float dt);