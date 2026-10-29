#include <assert.h>
#include <stdio.h>
#include <string.h>
#include DISTORTION_SOURCE
static void step(MasterDistortion *d,int16_t l,int16_t r,unsigned channels) {
    _Alignas(4) int16_t in[2]={l,r},out[2]={0};
    Distortion_process(d,in,out,channels==2?out+1:NULL,1,channels,1);
}
int main(void) {
    for(unsigned channels=1;channels<=2;channels++) {
        MasterDistortion d;memset(&d,0x55,sizeof d);Distortion_init(&d);
        assert(d.clipInputHp_state[0]==0 && d.clipInputHp_state[1]==0);
        d.distortionType=DISTORTION_RAT;d.drive=d.p1=d.p2=d.p3=1;
        step(&d,10000,-30000,channels);
        assert(d.clipInputHp_state[0]!=0 && d.clipFeedbackGain==1);
        double history=d.clipInputHp_state[0];
        Distortion_resetClip(&d);assert(d.clipInputHp_state[0]==history);
        step(&d,0,0,channels);
        assert(d.clipInputHp_state[0]==0 && d.clipInputHp_state[1]==0 && d.clipFeedbackGain==0);
        for(unsigned n=0;n<10000;n++)step(&d,0,0,channels);
        assert(d.lastOut[0]*32768>-1 && d.lastOut[0]*32768<1);
        step(&d,0,2,channels);assert(d.clipFeedbackGain==(channels==2?1:0));
        step(&d,2,0,channels);assert(d.clipFeedbackGain==1);
        for(unsigned n=0;n<441;n++)step(&d,1,-1,channels);
        assert(d.clipFeedbackGain==1);
        for(unsigned n=0;n<221;n++)step(&d,0,0,channels);
        assert(d.clipFeedbackGain>0 && d.clipFeedbackGain<1e-6);
        step(&d,0,0,channels);assert(d.clipFeedbackGain==0);
        /* Every non-CLP algorithm invalidates CLP entry, including new histories. */
        for(unsigned other=0;other<2;other++) {
            d.distortionType=other;step(&d,5000,-5000,channels);
            d.clipInputHp_state[0]=.5;d.clipInputHp_state[1]=-.3;
            d.distortionType=DISTORTION_RAT;step(&d,0,0,channels);
            assert(d.clipInputHp_state[0]==0 && d.clipInputHp_state[1]==0);
        }
    }
    puts("PASS R6 histories initialized/audio-owned reset; exact inherited hold/fade; original-bus reopen; mono/stereo; Tube/Fold re-entry");
}
