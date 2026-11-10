/* Two fixed automation targets per voice; runtime interpolation only. */
#ifndef LXR_AUTOMATION_PLAYBACK_H
#define LXR_AUTOMATION_PLAYBACK_H
#include <stdint.h>
#define AUTOMATION_PLAYBACK_NONE 255u
typedef struct {
    uint16_t destination;
    uint8_t from;
    uint8_t to;
    uint8_t divisions;
    uint8_t last_value;
    uint8_t last_slot;
} AutomationPlaybackLane;
/* Always publish the step boundary value. Intermediates are deduplicated. */
uint8_t automationPlayback_step(AutomationPlaybackLane *lane, uint16_t destination,
                                uint8_t from, uint8_t to, uint8_t *value);
uint8_t automationPlayback_tick(AutomationPlaybackLane *lane, uint8_t phase,
                                uint8_t *value);
#endif
