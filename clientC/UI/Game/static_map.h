// static_map.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct { uint16_t x,y,w,h; } CP_Rect;
typedef struct CP_Static {
  uint16_t nPlat, nVines, nWater;
  CP_Rect *plat, *vines, *water;
} CP_Static;

const CP_Static* cp_get_static(void);
void cp_free_static(void);
bool cp_recv_init_static_payload(const uint8_t *p, uint32_t len);
