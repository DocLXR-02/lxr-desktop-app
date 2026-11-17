/* P17/R23 experimental shared FX selection. GPL-3.0-only. */
#ifndef LXR_SHARED_FX_POOL_H_
#define LXR_SHARED_FX_POOL_H_
#include <stdint.h>

/* Existing audio destinations are 0..5; 6 is the legacy shared FX route.
 * Values 7..10 are not yet exposed by the menu or stored as new kit data.
 */
enum {
    FX_POOL_ROUTE_LEGACY = 6,
    FX_POOL_ROUTE_DELAY = 7,
    FX_POOL_ROUTE_RING = 8,
    FX_POOL_ROUTE_DISTORTION = 9,
    FX_POOL_ROUTE_COMPRESSOR = 10
};
/* Values intentionally match the existing FxTypeEnum. */
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

static inline uint8_t SharedFxPool_typeForRoute(uint8_t route, uint8_t legacyType)
{
    switch (route) {
    case FX_POOL_ROUTE_LEGACY:
        return legacyType < FX_POOL_PROCESSOR_COUNT ? legacyType : FX_POOL_INVALID;
    case FX_POOL_ROUTE_DELAY:
        return FX_POOL_TYPE_DELAY;
    case FX_POOL_ROUTE_RING:
        return FX_POOL_TYPE_RING;
    case FX_POOL_ROUTE_DISTORTION:
        return FX_POOL_TYPE_DISTORTION;
    case FX_POOL_ROUTE_COMPRESSOR:
        return FX_POOL_TYPE_COMPRESSOR;
    default:
        return FX_POOL_INVALID; /* Physical outputs bypass the shared FX pool. */
    }
}

/* Bitmask of unique effect processors, not the number of routed voices. */
static inline uint8_t SharedFxPool_activeMask(const uint8_t routes[FX_POOL_VOICE_COUNT],
                                               uint8_t legacyType)
{
    uint8_t mask = 0;
    for (uint8_t voice = 0; voice < FX_POOL_VOICE_COUNT; ++voice) {
        const uint8_t type = SharedFxPool_typeForRoute(routes[voice], legacyType);
        if (type != FX_POOL_INVALID)
            mask |= (uint8_t)(1u << type);
    }
    return mask;
}
#endif
