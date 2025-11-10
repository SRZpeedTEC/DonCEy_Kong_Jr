#include "static_map.h"
#include <stdlib.h>
#include <string.h>

// --- helpers endian locales ---
static inline uint16_t be16(const uint8_t *p){ return (uint16_t)((p[0]<<8)|p[1]); }

// --- lectura de estructuras ---
static int read_rect(const uint8_t **p, uint32_t *len, CP_Rect *r){
    if (*len < 8) return 0;
    const uint8_t *q = *p;
    r->x = be16(q); r->y = be16(q+2); r->w = be16(q+4); r->h = be16(q+6);
    *p += 8; *len -= 8; return 1;
}

static int read_rect_array(const uint8_t **p, uint32_t *len, uint16_t *n, CP_Rect **out){
    if (*len < 2) return 0;
    *n = be16(*p); *p += 2; *len -= 2;
    if (*n == 0){ *out = NULL; return 1; }
    uint32_t need = (uint32_t)(*n)*8u;
    if (*len < need) return 0;

    CP_Rect *arr = (CP_Rect*)malloc((*n)*sizeof(CP_Rect));
    if (!arr) return 0;

    for (uint16_t i=0;i<*n;i++){
        if (!read_rect(p,len,&arr[i])){ free(arr); return 0; }
    }
    *out = arr; return 1;
}

// --- buffer global del mapa estático ---
static CP_Static g_static = {0};

const CP_Static* cp_get_static(void){ return &g_static; }

void cp_free_static(void){
    free(g_static.plat);   g_static.plat   = NULL; g_static.nPlat   = 0;
    free(g_static.vines);  g_static.vines  = NULL; g_static.nVines  = 0;
    free(g_static.water);  g_static.water  = NULL; g_static.nWater  = 0;
}

// Payload INIT desde el server Java:
// player(8) + plat[] + vines[] + enemies[] + fruits[] + water[]
// Guardamos plat, vines y water. Enemies/fruits se descartan.
bool cp_recv_init_static_payload(const uint8_t *p, uint32_t len){
    cp_free_static();

    CP_Rect tmp;
    if (!read_rect(&p,&len,&tmp)) return false; // player: ignorado

    if (!read_rect_array(&p,&len,&g_static.nPlat,  &g_static.plat))   return false;
    if (!read_rect_array(&p,&len,&g_static.nVines, &g_static.vines))  return false;

    // enemies (descartar)
    uint16_t nE=0; CP_Rect *eTmp=NULL;
    if (!read_rect_array(&p,&len,&nE,&eTmp)) return false;
    free(eTmp);

    // fruits (descartar)
    uint16_t nF=0; CP_Rect *fTmp=NULL;
    if (!read_rect_array(&p,&len,&nF,&fTmp)) return false;
    free(fTmp);

    // water (guardar)
    if (!read_rect_array(&p,&len,&g_static.nWater, &g_static.water))  return false;

    return (len==0);
}
