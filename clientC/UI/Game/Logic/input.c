#include "input.h"
#include "raylib.h"

// Read keyboard and build a simple intent state.
InputState input_read(void) {
    InputState state;
    state.left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    state.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    state.up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    state.down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);

    // Treat jump as an edge (pressed this frame).
    // Space or Up trigger jump.
    state.jump  = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);

    return state;
}