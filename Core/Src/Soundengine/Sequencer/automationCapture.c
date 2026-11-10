/* BUG 008: sequencer-independent automation recording buffer. */
#include "Soundengine/Sequencer/automationCapture.h"
#include <string.h>
void autoCapture_arm(AutomationCapture *capture, uint8_t length, uint8_t known_target)
{
    memset(capture, 0, sizeof(*capture));
    if(length == 0 || length > AUTO_CAPTURE_MAX_STEPS) return;
    capture->active = 1;
    capture->length = length;
    capture->target = known_target;
}
void autoCapture_stop(AutomationCapture *capture)
{
    capture->active = 0;
    capture->started = 0;
    capture->changed = 0;
}
uint8_t autoCapture_step(AutomationCapture *capture, uint8_t new_step)
{
    if(!capture->active || new_step >= capture->length) return 0;
    if(!capture->started) {
        if(new_step == 0) {
            capture->started = 1;
            capture->current_step = 0;
        }
        return 0;
    }
    if(capture->changed)
        capture->values[capture->current_step] = capture->held_value;
    if(new_step == 0) {
        capture->current_step = 0;
        return capture->changed;
    }
    capture->current_step = new_step;
    if(capture->changed)
        capture->values[new_step] = capture->held_value;
    return 0;
}
uint8_t autoCapture_event(AutomationCapture *capture, uint8_t target,
                          uint8_t previous_value, uint8_t new_value)
{
    if(!capture->active || !capture->started || target == AUTO_CAPTURE_NO_TARGET ||
       previous_value == new_value)
        return 0;
    if(capture->target != AUTO_CAPTURE_NO_TARGET && capture->target != target)
        return 0;
    if(!capture->changed) {
        if(capture->target == AUTO_CAPTURE_NO_TARGET)
            capture->target = target;
        /* A late first movement preserves the initial value on earlier steps. */
        for(uint8_t step = 0; step < capture->current_step; step++)
            capture->values[step] = previous_value;
        capture->changed = 1;
    }
    capture->held_value = new_value;
    capture->values[capture->current_step] = new_value;
    return 1;
}
void autoCapture_committed(AutomationCapture *capture)
{
    capture->changed = 0;
}
