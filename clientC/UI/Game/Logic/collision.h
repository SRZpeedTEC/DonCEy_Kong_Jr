#pragma once
// Basic collision helpers between player and static platforms.

#include <stdbool.h>
#include "player.h"
#include "map.h"

typedef enum {
    COLLISION_PHASE_HORIZONTAL,
    COLLISION_PHASE_VERTICAL
} CollisionPhase;

// Simple integer AABB overlap test.
bool rects_overlap_i(int ax, int ay, int aw, int ah,
                     int bx, int by, int bw, int bh);

// Resolve player vs platforms for the given phase (horizontal or vertical).
void resolve_player_platform_collisions(Player* player,
                                        const MapView* mapView,
                                        CollisionPhase phase);

// Recompute grounded based on floor and platform tops.
void update_player_grounded(Player* player, const MapView* mapView,
                            int worldTop, int worldHeight);

// check if player touches any vine (does not resolve movement yet)
bool player_touching_vine(const Player* player, const MapView* map);