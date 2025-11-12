#pragma once
// Physics module: gravity, jump, bounds clamp (will be done in step 7).

#include "player.h"
#include "input.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

// Step physics for one frame.
// Params:
//   p  : in/out player state
//   in : input intents
//   map: map view (bounds, solids in future)
//   dt : delta time in seconds (or frames; we will define usage later)
void physics_step(Player* p, const InputState* in, const MapView* map, float dt);

#ifdef __cplusplus
}
#endif