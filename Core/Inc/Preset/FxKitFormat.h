/* Versioned, append-only FX extension for legacy LXR Kit (.snd) payloads.
 * The original eight-byte name and END_OF_SOUND_PARAMETERS byte prefix stay unchanged.
 * There is no change to the R18 CLP DSP algorithm or audio signal.
 */
#ifndef LXR_PRESET_FX_KIT_FORMAT_H
#define LXR_PRESET_FX_KIT_FORMAT_H
#include <stdint.h>
#include <string.h>

enum {
    FX_KIT_FIRST_TYPE = 1,
    FX_KIT_LAST_TYPE = 4,
    FX_KIT_EFFECT_COUNT = 4,
    FX_KIT_FIELDS_PER_EFFECT = 6, /* P1..P4, Dry/Wet, output */
    FX_KIT_PAYLOAD_BYTES = FX_KIT_EFFECT_COUNT * FX_KIT_FIELDS_PER_EFFECT,
    FX_KIT_HEADER_BYTES = 6, /* LXFX, version 1, payload length */
    FX_KIT_CRC_BYTES = 2,
    FX_KIT_EXTENSION_BYTES = FX_KIT_HEADER_BYTES + FX_KIT_PAYLOAD_BYTES + FX_KIT_CRC_BYTES
};

static inline uint16_t fxKit_crc16(const uint8_t *bytes, unsigned count)
{
    uint16_t crc = 0xffffu;
    for (unsigned i = 0; i < count; i++) {
        crc ^= (uint16_t)bytes[i] << 8;
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (uint16_t)((crc & 0x8000u) ?
                (((uint32_t)crc << 1u) ^ 0x1021u) :
                ((uint32_t)crc << 1u));
    }
    return crc;
}

/* A complete extension is either fully valid or entirely rejected. */
static inline uint8_t fxKit_validate(const uint8_t block[FX_KIT_EXTENSION_BYTES])
{
    if (!block || memcmp(block, "LXFX", 4) ||
        block[4] != 1u || block[5] != FX_KIT_PAYLOAD_BYTES)
        return 0u;
    const uint16_t actual = fxKit_crc16(block, FX_KIT_HEADER_BYTES + FX_KIT_PAYLOAD_BYTES);
    const uint16_t stored = (uint16_t)block[FX_KIT_EXTENSION_BYTES-2] |
                            (uint16_t)block[FX_KIT_EXTENSION_BYTES-1] << 8;
    if (actual != stored)
        return 0u;
    for (unsigned effect = 0; effect < FX_KIT_EFFECT_COUNT; effect++) {
        const unsigned at = FX_KIT_HEADER_BYTES + effect * FX_KIT_FIELDS_PER_EFFECT;
        for (unsigned p = 0; p < 4; p++)
            if (block[at+p] > 127u)
                return 0u;
        if (block[at+4] > 100u || block[at+5] > 5u)
            return 0u;
    }
    return 1u;
}

static inline void fxKit_writeBlock(uint8_t block[FX_KIT_EXTENSION_BYTES],
                                    const uint8_t values[FX_KIT_PAYLOAD_BYTES])
{
    memcpy(block, "LXFX", 4);
    block[4] = 1u;
    block[5] = FX_KIT_PAYLOAD_BYTES;
    memcpy(block + FX_KIT_HEADER_BYTES, values, FX_KIT_PAYLOAD_BYTES);
    const uint16_t crc = fxKit_crc16(block, FX_KIT_HEADER_BYTES + FX_KIT_PAYLOAD_BYTES);
    block[FX_KIT_EXTENSION_BYTES-2] = (uint8_t)crc;
    block[FX_KIT_EXTENSION_BYTES-1] = (uint8_t)(crc >> 8);
}

static inline void fxKit_readValues(uint8_t values[FX_KIT_PAYLOAD_BYTES],
                                    const uint8_t block[FX_KIT_EXTENSION_BYTES])
{
    memcpy(values, block + FX_KIT_HEADER_BYTES, FX_KIT_PAYLOAD_BYTES);
}

#endif /* LXR_PRESET_FX_KIT_FORMAT_H */
