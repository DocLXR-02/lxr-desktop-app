/* BUG 008: bounded one-parameter one-loop automation capture. */
#ifndef LXR_AUTOMATION_CAPTURE_H
#define LXR_AUTOMATION_CAPTURE_H
#include <stdint.h>
#define AUTO_CAPTURE_MAX_STEPS 64u
#define AUTO_CAPTURE_NO_TARGET 255u
typedef struct {
    uint8_t active;
    uint8_t started;
    uint8_t changed;
    uint8_t length;
    uint8_t current_step;
    uint8_t target;
    uint8_t held_value;
    uint8_t values[AUTO_CAPTURE_MAX_STEPS];
} AutomationCapture;
void autoCapture_arm(AutomationCapture *capture, uint8_t length, uint8_t known_target);
void autoCapture_stop(AutomationCapture *capture);
/* Return 1 only when a complete changed loop is ready to commit. */
uint8_t autoCapture_step(AutomationCapture *capture, uint8_t new_step);
/* Return 1 only when an input parameter event was accepted. */
uint8_t autoCapture_event(AutomationCapture *capture, uint8_t target,
                          uint8_t previous_value, uint8_t new_value);
void autoCapture_committed(AutomationCapture *capture);
#endif
