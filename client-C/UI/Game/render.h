#pragma once
#include "proto.h"

void render_init(int vw, int vh, int scale);
void render_shutdown(void);
void render_frame_static(const CP_Static* s);

// fondo guía
void render_set_bg(const char *path, float alpha);
void render_clear_bg(void);
