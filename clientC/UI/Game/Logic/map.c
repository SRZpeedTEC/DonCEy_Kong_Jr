// Map adapter implementation (skeleton).
#include "map.h"
#include "../static_map.h"

MapView map_view_build(void) {
    MapView v;
    v.data = cp_get_static();
    return v;
}

void map_get_world_bounds(int* outX, int* outY, int* outW, int* outH) {
    if (outX) *outX = 0;
    if (outY) *outY = 0;
    if (outW) *outW = 256;  // matches VW from game_init
    if (outH) *outH = 240;  // matches VH from game_init
}