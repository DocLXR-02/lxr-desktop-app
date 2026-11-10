/* Bounded, allocation-free 24-phase sequencer automation interpolation. */
#include "Soundengine/Sequencer/automationPlayback.h"
uint8_t automationPlayback_step(AutomationPlaybackLane *lane, uint16_t destination,
                                uint8_t from, uint8_t to, uint8_t *value)
{
    if(!lane || !value || destination == AUTOMATION_PLAYBACK_NONE ||
       destination == 0u || from > 127u || to > 127u) return 0;
    lane->destination = destination;
    lane->from = from;
    lane->to = to;
    lane->divisions = 8u; /* Fixed interpolation, validated by owner listening. */
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
    const uint8_t slot = (uint8_t)(phase / 3u);
    if(!slot || slot >= 8u || slot <= lane->last_slot)
        return 0;
    lane->last_slot = slot;
    const int16_t delta = (int16_t)lane->to - (int16_t)lane->from;
    const uint8_t next = (uint8_t)((int16_t)lane->from +
                                   (delta * (int16_t)slot) / 8);
    if(next == lane->last_value) return 0;
    lane->last_value = next;
    *value = next;
    return 1;
}
