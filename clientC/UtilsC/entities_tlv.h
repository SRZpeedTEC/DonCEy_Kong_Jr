// UtilsC/entities_tlv.h
#pragma once
#include <stdint.h>
#include <stddef.h>

// entity kinds sent inside TLV_ENTITIES_CORR
enum {
    ENTITY_KIND_PLAYER = 0,
    ENTITY_KIND_CROC   = 1,
    ENTITY_KIND_FRUIT  = 2
};

// small snapshot of an entity used only for serialization
typedef struct {
    uint8_t kind;      // one of ENTITY_KIND_*
    uint8_t spriteId;  // animation / frame id decided by gameplay
    int16_t x;         // world position (pixels)
    int16_t y;         // world position (pixels)
} EntitySnapshot;

/**
 * Build a TLV_ENTITIES_CORR into dst.
 *
 * dst          : output buffer
 * dstCapacity  : size of dst in bytes
 * entities     : array of entity snapshots
 * entityCount  : number of entities (0..255)
 *
 * Returns total TLV size in bytes (type + length + value),
 * or 0 if there is not enough space or invalid params.
 */
size_t entities_tlv_build(uint8_t *dst,
                          size_t dstCapacity,
                          const EntitySnapshot *entities,
                          uint8_t entityCount);