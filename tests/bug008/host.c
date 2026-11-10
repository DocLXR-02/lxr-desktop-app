#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "Soundengine/Sequencer/automationCapture.h"
static void advance(AutomationCapture *c, unsigned n)
{
    assert(n < c->length);
    assert(autoCapture_step(c, (uint8_t)n) == 0);
}
int main(void)
{
    AutomationCapture c;
    autoCapture_arm(&c, 16, AUTO_CAPTURE_NO_TARGET);
    for(unsigned i=0;i<16;i++) advance(&c, i);
    assert(autoCapture_step(&c, 0) == 0); /* no input, no commit */
    assert(autoCapture_event(&c, 10, 40, 40) == 1);
    for(unsigned i=1;i<=4;i++) advance(&c, i);
    assert(autoCapture_event(&c, 10, 40, 70) == 1);
    assert(autoCapture_event(&c, 11, 20, 90) == 0);
    for(unsigned i=5;i<16;i++) advance(&c, i);
    assert(autoCapture_step(&c, 0) == 1);
    for(unsigned i=0;i<4;i++) assert(c.values[i] == 40);
    for(unsigned i=4;i<16;i++) assert(c.values[i] == 70);
    autoCapture_committed(&c);
    for(unsigned i=1;i<16;i++) advance(&c, i);
    assert(autoCapture_step(&c, 0) == 0);
    for(unsigned i=1;i<=8;i++) advance(&c, i);
    assert(autoCapture_event(&c, 10, 70, 20) == 1);
    for(unsigned i=9;i<16;i++) advance(&c, i);
    assert(autoCapture_step(&c, 0) == 1);
    for(unsigned i=0;i<8;i++) assert(c.values[i] == 70);
    for(unsigned i=8;i<16;i++) assert(c.values[i] == 20);
    autoCapture_committed(&c);
    autoCapture_stop(&c);
    assert(autoCapture_event(&c, 10, 20, 50) == 0);
    autoCapture_arm(&c, 16, 17);
    for(unsigned i=0;i<16;i++) advance(&c, i);
    assert(autoCapture_event(&c, 17, 5, 65) == 1);
    assert(autoCapture_step(&c, 0) == 1);
    for(unsigned i=0;i<15;i++) assert(c.values[i] == 5);
    assert(c.values[15] == 65);
    autoCapture_committed(&c);
    assert(autoCapture_event(&c, 17, 65, 30) == 1);
    autoCapture_stop(&c);
    assert(autoCapture_step(&c, 0) == 0);
    puts("BUG008 capture: core scenarios PASS");
}
