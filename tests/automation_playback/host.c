/* Fixed-8 automation regression and exhaustive equivalence to the accepted A/B FIX8 formula. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "Soundengine/Sequencer/automationPlayback.h"

static unsigned long transitions;

static void verify_pair(uint8_t from, uint8_t to)
{
    AutomationPlaybackLane lane = {0};
    uint8_t out = 255u;
    assert(automationPlayback_step(&lane, 18u, from, to, &out) == 1u);
    assert(out == from);
    assert(lane.divisions == 8u);
    uint8_t old_last_value = from;
    uint8_t old_last_slot = 0u;
    for(uint8_t phase = 0u; phase < 24u; ++phase) {
        const uint8_t old_slot = (uint8_t)(phase / 3u);
        uint8_t old_changed = 0u;
        uint8_t old_value = old_last_value;
        if(old_slot > 0u && old_slot < 8u && old_slot > old_last_slot) {
            old_last_slot = old_slot;
            const int16_t delta = (int16_t)to - (int16_t)from;
            const uint8_t interpolated =
                (uint8_t)((int16_t)from + (delta * (int16_t)old_slot) / 8);
            if(interpolated != old_last_value) {
                old_last_value = interpolated;
                old_value = interpolated;
                old_changed = 1u;
            }
        }
        const uint8_t changed = automationPlayback_tick(&lane, phase, &out);
        assert(changed == old_changed);
        if(changed) {
            assert(out == old_value);
            transitions++;
        }
        assert(automationPlayback_tick(&lane, phase, &out) == 0u);
    }
    assert(automationPlayback_tick(&lane, 24u, &out) == 0u);
    /* Explicit next step boundary reasserts exact target. */
    assert(automationPlayback_step(&lane, 18u, to, from, &out) == 1u);
    assert(out == to);
}

int main(void)
{
    AutomationPlaybackLane lane = {0};
    uint8_t out = 0u;
    for(unsigned from = 0; from <= 127u; ++from)
        for(unsigned to = 0; to <= 127u; ++to)
            verify_pair((uint8_t)from, (uint8_t)to);
    assert(!automationPlayback_step(&lane, AUTOMATION_PLAYBACK_NONE, 20, 30, &out));
    assert(!automationPlayback_step(&lane, 0, 20, 30, &out));
    assert(!automationPlayback_step(&lane, 18, 128, 30, &out));
    assert(!automationPlayback_step(&lane, 18, 20, 128, &out));
    assert(!automationPlayback_tick(&lane, 24, &out));
    assert(automationPlayback_step(&lane, 18, 30, 90, &out));
    lane.divisions = 0u; /* Armed REC suppresses incomplete-take playback. */
    assert(!automationPlayback_tick(&lane, 3u, &out));
    printf("FIX8: PASS exhaustive 16384 start/end pairs, identical intermediate events; %lu changes\n",
           transitions);
    return 0;
}
