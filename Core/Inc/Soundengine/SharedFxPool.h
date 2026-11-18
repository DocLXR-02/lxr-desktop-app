/* R23 shared FX pool: direct processor routing and one-time stored-Kit migration.
 * GPL-3.0-only. The historical voice route 6 is reserved, not a DSP route.
 */
#ifndef LXR_SHARED_FX_POOL_H_
#define LXR_SHARED_FX_POOL_H_
#include <stdint.h>

/* Physical destinations are 0..5. Keep 7..10 stable for existing R23 Kits. */
enum {
    FX_POOL_ROUTE_DELAY = 7,
    FX_POOL_ROUTE_RING = 8,
    FX_POOL_ROUTE_DISTORTION = 9,
    FX_POOL_ROUTE_COMPRESSOR = 10
};
/* Processor IDs match FxTypeEnum, independent of the selected editor. */
enum {
    FX_POOL_TYPE_OFF = 0,
    FX_POOL_TYPE_DISTORTION = 1,
    FX_POOL_TYPE_RING = 2,
    FX_POOL_TYPE_COMPRESSOR = 3,
    FX_POOL_TYPE_DELAY = 4,
    FX_POOL_PROCESSOR_COUNT = 5,
    FX_POOL_VOICE_COUNT = 6,
    FX_POOL_INVALID = 255
};

/* Used only when decoding stored Kits, never by the audio mixer.
 * A historical route 6 followed the then-selected effect.
 * FX Off was a dry pass-through; use the stored physical FX output for bypass.
 * Preserve already-direct R23 route numbers without renumbering.
 */
static inline uint8_t SharedFxPool_upgradeStoredRoute(uint8_t storedRoute,
                                                       uint8_t storedFxType,
                                                       uint8_t storedFxOutput)
{
    if (storedRoute == 6u) {
        switch (storedFxType) {
        case FX_POOL_TYPE_DISTORTION: return FX_POOL_ROUTE_DISTORTION;
        case FX_POOL_TYPE_RING: return FX_POOL_ROUTE_RING;
        case FX_POOL_TYPE_COMPRESSOR: return FX_POOL_ROUTE_COMPRESSOR;
        case FX_POOL_TYPE_DELAY: return FX_POOL_ROUTE_DELAY;
        default: return storedFxOutput <= 5u ? storedFxOutput : 0u;
        }
    }
    return storedRoute <= FX_POOL_ROUTE_COMPRESSOR ? storedRoute : 0u;
}

/* No selected-effect or global legacy route in the real-time audio path. */
static inline uint8_t SharedFxPool_typeForRoute(uint8_t route)
{
    switch (route) {
    case FX_POOL_ROUTE_DELAY: return FX_POOL_TYPE_DELAY;
    case FX_POOL_ROUTE_RING: return FX_POOL_TYPE_RING;
    case FX_POOL_ROUTE_DISTORTION: return FX_POOL_TYPE_DISTORTION;
    case FX_POOL_ROUTE_COMPRESSOR: return FX_POOL_TYPE_COMPRESSOR;
    default: return FX_POOL_INVALID; /* 0..6 and invalid bypass FX processors. */
    }
}

/* A processor executes at most once per block for all its routed voices. */
static inline uint8_t SharedFxPool_activeMask(const uint8_t routes[FX_POOL_VOICE_COUNT])
{
    uint8_t mask = 0;
    for (uint8_t voice = 0; voice < FX_POOL_VOICE_COUNT; ++voice) {
        const uint8_t type = SharedFxPool_typeForRoute(routes[voice]);
        if (type != FX_POOL_INVALID)
            mask |= (uint8_t)(1u << type);
    }
    return mask;
}
#endif
