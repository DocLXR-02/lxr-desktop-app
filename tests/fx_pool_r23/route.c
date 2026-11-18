/* R23 direct shared FX pool routing and archived Kit decoding host oracle. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "Soundengine/SharedFxPool.h"

static uint8_t expected_type(uint8_t route)
{
    switch(route) {
    case 7: return 4;
    case 8: return 2;
    case 9: return 1;
    case 10: return 3;
    default: return 255;
    }
}
static uint8_t expected_legacy(uint8_t fx, uint8_t physical)
{
    switch(fx) {
    case 1: return 9;
    case 2: return 8;
    case 3: return 10;
    case 4: return 7;
    default: return physical <= 5 ? physical : 0;
    }
}
int main(void)
{
    const uint8_t choices[] = {0, 6, 7, 8, 9, 10};
    unsigned long cases = 0;
    /* No global FX selector can change the real-time type lookup. */
    for (unsigned route = 0; route < 256; route++) {
        assert(SharedFxPool_typeForRoute((uint8_t)route) == expected_type((uint8_t)route));
        cases++;
    }
    assert(SharedFxPool_typeForRoute(6) == FX_POOL_INVALID);
    for (unsigned fx = 0; fx < 256; fx++) {
        for (unsigned physical = 0; physical < 8; physical++) {
            uint8_t decoded = SharedFxPool_upgradeStoredRoute(6, (uint8_t)fx, (uint8_t)physical);
            assert(decoded == expected_legacy((uint8_t)fx, (uint8_t)physical));
            assert(decoded != 6);
            cases++;
        }
        for (unsigned route = 0; route < 256; route++) {
            if (route == 6) continue;
            const uint8_t result=SharedFxPool_upgradeStoredRoute((uint8_t)route,(uint8_t)fx,2);
            assert(result == (route <= 10 ? route : 0));
            assert(result != 6);
            cases++;
        }
    }
    /* All 6^6 direct/invalid routes, without a selected-effect parameter. */
    for (uint32_t combination = 0; combination < 46656u; combination++) {
        uint32_t value = combination;
        uint8_t routes[6], expectedMask = 0;
        for (unsigned voice = 0; voice < 6; voice++) {
            routes[voice] = choices[value % 6u];
            value /= 6u;
            uint8_t type = expected_type(routes[voice]);
            if(type < 5u)expectedMask |= (uint8_t)(1u << type);
        }
        assert(SharedFxPool_activeMask(routes) == expectedMask);
        cases++;
    }
    { const uint8_t allDelay[6]={7,7,7,7,7,7};
      assert(SharedFxPool_activeMask(allDelay)==(1u<<4)); }
    { const uint8_t allEffects[6]={7,8,9,10,0,6};
      assert(SharedFxPool_activeMask(allEffects)==0x1eu); }
    printf("PASS R23 direct-FX pool: %lu routing/upgrade/mask cases; archived 6 never dispatched\n",cases);
    return 0;
}
