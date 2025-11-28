#pragma once
#include "../Game/static_map.h"

#include "Logic/player.h"
#include "Logic/crocodile.h"
#include "Logic/fruit.h"
#include "Logic/constants.h"


// Forward declarations to avoid pulling heavy headers
struct Player;
struct Crocodile;
struct Fruit;
struct CP_Static;

// Load and unload sprite textures
void render_init_assets(void);
void render_shutdown_assets(void);
  

// Load and unload sprite textures
void render_init_assets(void);
void render_shutdown_assets(void);

// Draw static map + entities (player, crocs, fruits)
void render_draw_level(const struct CP_Static* s,
                       const Player*    player,
                       const Crocodile* crocs,  int crocCount,
                       const Fruit*     fruits, int fruitCount);