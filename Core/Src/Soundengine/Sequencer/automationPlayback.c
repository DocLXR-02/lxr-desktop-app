/* Bounded, allocation-free 24-phase sequencer automation interpolation. */
#include "Soundengine/Sequencer/automationPlayback.h"
static uint8_t chooseDivisions(uint8_t from, uint8_t to,
                               uint8_t resolution, uint8_t adaptive)
{
    const uint8_t maximum = (uint8_t)(1u << (resolution > 3 ? 3 : resolution));
    if(!adaptive) return maximum;
    const uint8_t delta = from > to ? (uint8_t)(from-to) : (uint8_t)(to-from);
    uint8_t divisions = 1;
    while(divisions < maximum && delta > 4u * divisions)
        divisions = (uint8_t)(divisions << 1);
    return divisions;
}
uint8_t automationPlayback_step(AutomationPlaybackLane *lane, uint16_t destination,
                                uint8_t from, uint8_t to, uint8_t resolution,
                                uint8_t adaptive, uint8_t *value)
{
    if(!lane || !value || destination == AUTOMATION_PLAYBACK_NONE ||
       destination == 0u || from > 127u || to > 127u) return 0;
    lane->destination = destination;
    lane->from = from;
    lane->to = to;
    lane->divisions = chooseDivisions(from, to, resolution, adaptive);
    lane->last_slot = 0u;
    lane->last_value = from;
    *value = from;
    return 1;
}
uint8_t automationPlayback_tick(AutomationPlaybackLane *lane, uint8_t phase,
                                uint8_t *value)
{
    if(!lane || !value || !lane->divisions || phase >= 24u) return 0;
    /* All divisions exactly partition the existing sequencer's 24 phases. */
    const uint8_t ticksPerUpdate = (uint8_t)(24u / lane->divisions);
    const uint8_t slot = (uint8_t)(phase / ticksPerUpdate);
    if(!slot || slot >= lane->divisions || slot <= lane->last_slot)
        return 0;
    lane->last_slot = slot;
    const int16_t delta = (int16_t)lane->to - (int16_t)lane->from;
    const uint8_t next = (uint8_t)((int16_t)lane->from +
                                   (delta * (int16_t)slot) / (int16_t)lane->divisions);
    if(next == lane->last_value) return 0;
    lane->last_value = next;
    *value = next;
    return 1;
}
