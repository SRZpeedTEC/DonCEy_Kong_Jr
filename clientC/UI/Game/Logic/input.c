#include "input.h"
#include "raylib.h"

// Read keyboard and build a simple intent state.
InputState input_read(void) {
    InputState s;
    s.left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    s.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    s.up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    s.down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);

    // Treat jump as an edge (pressed this frame).
    // Space or Up trigger jump.
    s.jump  = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);

    return s;
}