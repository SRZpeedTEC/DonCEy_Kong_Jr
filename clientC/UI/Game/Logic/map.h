#pragma once
// Map adapter module: convenience queries over CP_Static.

#include <stdbool.h>
#include <stdint.h>

struct CP_Static;

typedef struct {
    // In the future we could cache bounds or grid here.
    // For now, this is just a view over CP_Static.
    const struct CP_Static* data;
} MapView;

// Build a MapView reading from static_map.
MapView map_view_build(void);

// Query the level rectangle bounds (viewport/world space).
// For now we will return the canonical 256x240 (NES-like) viewport.
void map_get_world_bounds(int* outX, int* outY, int* outW, int* outH);
