#pragma once
// Basic collision helpers between player and static platforms.

#include <stdbool.h>
#include "player.h"
#include "map.h"
#include "fruit.h"

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

// check if there is at least one vine to the left and one to the right
// at roughly the same vertical range as the player
bool player_between_vines(const Player* player,
                          const MapView* map);

// find the index of the vine the player is currently overlapping
int collision_find_current_vine_index(const Player* player, const MapView* map);

// find a neighbor vine that can be reached by stretching left/right
// direction: -1 = reach left, +1 = reach right
int collision_find_neighbor_vine_reachable(const Player* player, const MapView* map, int currentVineIndex, int direction);

// updates Jr state when grabbed to two vines
void collision_update_between_vines_state(Player* player, const MapView* map);

bool player_hits_water(const Player* player, const MapView* map);

// true if player overlaps mario kill rectangle
bool player_touching_mario(const Player* player);

bool player_pick_fruits(Player* player, Fruit* fruits, int count);