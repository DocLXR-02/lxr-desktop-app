#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "Soundengine/Sequencer/automationPlayback.h"
static unsigned run(uint8_t max, uint8_t adaptive, uint8_t from, uint8_t to)
{
    AutomationPlaybackLane lane = {0};
    uint8_t out = 255;
    unsigned events = 0;
    assert(automationPlayback_step(&lane, 18, from, to, max, adaptive, &out));
    assert(out == from);
    for(unsigned phase=1;phase<24;phase++) {
        if(automationPlayback_tick(&lane, phase, &out)) {
            events++;
            assert(out <= 127);
        }
    }
    return events;
}
int main(void)
{
    AutomationPlaybackLane lane = {0};
    uint8_t out=0;
    for(unsigned n=0;n<4;n++) {
        assert(run(n,0,0,120) <= ((1u<<n)-1u));
        assert(run(n,1,0,120) <= ((1u<<n)-1u));
    }
    assert(run(3,0,70,70) == 0);
    assert(run(3,1,70,70) == 0);
    assert(run(3,1,70,73) == 0);
    assert(run(3,1,70,78) <= 1);
    assert(run(3,1,70,125) <= 7);
    assert(run(3,0,127,0) == 7);
    assert(run(3,0,0,127) == 7);
    assert(automationPlayback_step(&lane, 16, 20, 110, 3, 0, &out));
    for(unsigned phase=1;phase<24;phase++)
        if(automationPlayback_tick(&lane,(uint8_t)phase,&out)) assert(out<=110);
    assert(out<110);
    /* Next step boundary is always exact and globally reasserts value. */
    assert(automationPlayback_step(&lane, 16, 110, 20, 3, 0, &out));
    assert(out==110);
    assert(!automationPlayback_tick(&lane,0,&out));
    assert(!automationPlayback_tick(&lane,0,&out));
    assert(!automationPlayback_step(&lane,255,20,30,3,0,&out));
    assert(!automationPlayback_step(&lane,0,20,30,3,0,&out));
    assert(!automationPlayback_step(&lane,12,128,20,3,0,&out));
    assert(!automationPlayback_tick(&lane,24,&out));
    puts("AUTOMATION PLAYBACK: PASS boundary, interpolation, adaptive, 7-bit, inactive, duplicates");
    return 0;
}
