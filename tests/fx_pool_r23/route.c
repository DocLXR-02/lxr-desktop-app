/* R23 shared FX pool routing tests, isolated from STM32 hardware. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "Soundengine/SharedFxPool.h"

static uint8_t expected_type(uint8_t route, uint8_t legacy)
{
    if (route == 6) return legacy <= 4 ? legacy : 255;
    if (route == 7) return 4;
    if (route == 8) return 2;
    if (route == 9) return 1;
    if (route == 10) return 3;
    return 255;
}
int main(void)
{
    const uint8_t choices[] = {0, 6, 7, 8, 9, 10};
    unsigned long cases = 0;
    for (uint8_t legacy = 0; legacy <= 5; legacy++) {
        for (uint8_t route = 0; route <= 15; route++) {
            assert(SharedFxPool_typeForRoute(route, legacy) == expected_type(route, legacy));
            cases++;
        }
        for (uint32_t combination = 0; combination < 46656u; combination++) {
            uint32_t value = combination;
            uint8_t routes[6], expectedMask = 0;
            for (unsigned v = 0; v < 6; v++) {
                routes[v] = choices[value % 6u];
                value /= 6u;
                uint8_t type = expected_type(routes[v], legacy);
                if (type < 5u) expectedMask |= (uint8_t)(1u << type);
            }
            assert(SharedFxPool_activeMask(routes, legacy) == expectedMask);
            cases++;
        }
    }
    {   /* All six voices to Delay require exactly one shared processor. */
        const uint8_t allDelay[6] = {7,7,7,7,7,7};
        assert(SharedFxPool_activeMask(allDelay, 0) == (1u << 4));
    }
    {   /* Delay, Ring and Distortion are simultaneously dispatchable. */
        const uint8_t mixed[6] = {7,7,9,7,8,7};
        assert(SharedFxPool_activeMask(mixed, 0) == ((1u << 4)|(1u << 2)|(1u << 1)));
    }
    printf("PASS R23 Shared FX Pool: %lu route/mask cases, shared-once dispatch\n", cases);
    return 0;
}
