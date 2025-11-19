// UtilsC/entities_tlv.c
#include "entities_tlv.h"
#include "msg_types.h"   // for TLV_ENTITIES_CORR

// write u16 big-endian into buffer
static void write_u16_be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

size_t entities_tlv_build(uint8_t *dst,
                          size_t dstCapacity,
                          const EntitySnapshot *entities,
                          uint8_t entityCount)
{
    if (!dst || !entities) return 0;

    // value = 1 byte count + N * 6 bytes per entity
    uint16_t valueLen = (uint16_t)(1u + (uint16_t)entityCount * 6u);

    // TLV total = 1 (type) + 2 (length) + valueLen
    size_t totalLen = (size_t)3u + (size_t)valueLen;

    if (dstCapacity < totalLen) {
        return 0; // not enough space
    }

    // type
    dst[0] = (uint8_t)TLV_ENTITIES_CORR;

    // length (big-endian)
    write_u16_be(dst + 1, valueLen);

    // value begins at dst[3]
    uint8_t *p = dst + 3;

    // entityCount
    *p++ = entityCount;

    // entities
    for (uint8_t i = 0; i < entityCount; ++i) {
        const EntitySnapshot *e = &entities[i];

        // kind
        *p++ = e->kind;

        // spriteId
        *p++ = e->spriteId;

        // posX (i16 BE)
        write_u16_be(p, (uint16_t)e->x);
        p += 2;

        // posY (i16 BE)
        write_u16_be(p, (uint16_t)e->y);
        p += 2;
    }

    return totalLen;
}