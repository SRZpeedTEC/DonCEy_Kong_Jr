#pragma once
#include <stdbool.h>

typedef struct {
    bool left;
    bool right;
    bool up;
    bool down;
    bool jump;   // edge (pressed this frame)
} InputState;

InputState input_read(void);