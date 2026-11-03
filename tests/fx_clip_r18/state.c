/* R18: exercise actual CLP DSP after removal of the R7 parallel bass path. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include DISTORTION_SOURCE
static double peak;
static void step(MasterDistortion *d,int16_t l,int16_t r,unsigned channels) {
    _Alignas(4) int16_t in[2]={l,r},copy[2]={l,r},out[2]={0};
    Distortion_process(d,in,out,channels==2?out+1:NULL,1,channels,1);
    assert(!memcmp(in,copy,sizeof in));
    for(unsigned c=0;c<channels;c++) {
        double wet=d->lastOut[c];
        assert(isfinite(wet) && fabs(wet)<.64749);
        if(fabs(wet)>peak)peak=fabs(wet);
    }
}
int main(void) {
    for(unsigned channels=1;channels<=2;channels++) {
        MasterDistortion d;memset(&d,0x55,sizeof d);Distortion_init(&d);
        assert(d.hp1_state[0]==0 && d.hp1_state[1]==0);
        d.distortionType=DISTORTION_RAT;d.drive=d.p1=d.p2=d.p3=1;
        step(&d,10000,-30000,channels);
        assert(d.clipFeedbackGain==1);
        d.hp1_state[0]=.5;
        double history=d.hp1_state[0];
        Distortion_resetClip(&d);assert(d.hp1_state[0]==history);
        step(&d,0,0,channels);
        assert(d.hp1_state[0]==0 && d.hp1_state[1]==0 && d.clipFeedbackGain==0);
        for(unsigned n=0;n<10000;n++)step(&d,0,0,channels);
        assert(fabs(d.lastOut[0]*32768)<1);
        step(&d,0,2,channels);assert(d.clipFeedbackGain==(channels==2?1:0));
        step(&d,2,0,channels);assert(d.clipFeedbackGain==1);
        for(unsigned n=0;n<441;n++)step(&d,1,-1,channels);
        assert(d.clipFeedbackGain==1);
        for(unsigned n=0;n<221;n++)step(&d,0,0,channels);
        assert(d.clipFeedbackGain>0 && d.clipFeedbackGain<1e-6);
        step(&d,0,0,channels);assert(d.clipFeedbackGain==0);
        for(unsigned gap=440;gap<=1000;gap+=10) {
            step(&d,2,-2,channels);
            for(unsigned n=0;n<gap;n++)step(&d,0,0,channels);
            step(&d,-2,2,channels);assert(d.clipFeedbackGain==1);
        }
        for(unsigned other=0;other<2;other++) {
            d.distortionType=other;step(&d,5000,-5000,channels);
            d.hp1_state[0]=.5;d.hp1_state[1]=-.3;
            d.distortionType=DISTORTION_RAT;step(&d,0,0,channels);
            assert(d.hp1_state[0]==0 && d.hp1_state[1]==0);
        }
        for(unsigned n=0;n<88280;n++) {
            int16_t x=n<22070?-32768:n<44140?32767:(int16_t)(32767*sin(n*.00854));
            step(&d,x,(int16_t)-((int32_t)x/2),channels);
        }
        for(unsigned n=0;n<10000;n++)step(&d,0,0,channels);
        assert(fabs(d.lastOut[0]*32768)<1);
    }
    printf("PASS R18 input preservation, init/audio reset, detector/reopen, core headroom, idle; max wet %.9f\n",peak);
}
