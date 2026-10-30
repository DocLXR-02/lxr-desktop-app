#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
/* Reuse all original six-output-route assertions and other-FX link stubs. */
#define main inherited_bus_main
#include "../fx_clip/fxbus.c"
#undef main

uint8_t mixer_audioRouting[6];
struct { OnePoleFilter FaderVolumeFilter; } voiceArray[3], snareVoice, cymbalVoice, hatVoice;
static float targets[6];
float adc_getFaderVolume(uint8_t voice){return targets[voice];}
/* Actual mixer capture function, extracted verbatim by run.py. */
#include "capture.inc"
static OnePoleFilter *live[6];
static void init(void) {
    live[0]=&voiceArray[0].FaderVolumeFilter;live[1]=&voiceArray[1].FaderVolumeFilter;
    live[2]=&voiceArray[2].FaderVolumeFilter;live[3]=&snareVoice.FaderVolumeFilter;
    live[4]=&cymbalVoice.FaderVolumeFilter;live[5]=&hatVoice.FaderVolumeFilter;
    FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.distortion.distortionType=DISTORTION_RAT;
    fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=fxBus.distortion.p3=1;
    fxBus.dryWet=1;
    for(unsigned v=0;v<6;v++){
        live[v]->r=v<3?.01:.1;live[v]->b=.1+.1*v;
        targets[v]=.25;mixer_audioRouting[v]=MIXER_ROUTING_DAC1_STEREO;
    }
}
static float trajectory[3][4096];
int main(int argc,char **argv) {
    FILE *csv=argc>1?fopen(argv[1],"w"):NULL;
    if(argc>1)assert(csv);
    if(csv)fputs("voice,frame,gain\n",csv);
    inherited_bus_main();init();
    /* All route masks; silence, gain zero or velocity are not eligibility inputs. */
    for(unsigned mask=0;mask<64;mask++){
        unsigned count=0,sole=0;
        for(unsigned v=0;v<6;v++)if(mask&(1u<<v)){mixer_audioRouting[v]=MIXER_ROUTING_FX_BUS;count++;sole=v;}
        else mixer_audioRouting[v]=MIXER_ROUTING_DAC1_STEREO;
        double before[6];for(unsigned v=0;v<6;v++)before[v]=live[v]->b;
        const float *g=mixer_clipFaderForBlock();assert((g!=NULL)==(count==1));
        for(unsigned v=0;v<6;v++)assert(before[v]==live[v]->b);
        if(g){OnePoleFilter ref=*live[sole];for(unsigned n=0;n<64;n++)assert(g[n]==calcOnePole(&ref,targets[sole]));}
    }
    for(unsigned other=0;other<=4;other++)if(other!=FX_DISTORTION){fxBus.fxType=other;assert(!mixer_clipFaderForBlock());}
    fxBus.fxType=FX_DISTORTION;
    for(unsigned kind=0;kind<2;kind++){fxBus.distortion.distortionType=kind;assert(!mixer_clipFaderForBlock());}
    for(unsigned voice=0;voice<6;voice++)for(unsigned partition=0;partition<3;partition++){
        init();unsigned block=partition==0?1:partition==1?7:64;
        for(unsigned n=0;n<4096;){
            unsigned end=n<400?400:n<1900?1900:n<2300?2300:4096;
            unsigned size=end-n<block?end-n:block;
            targets[voice]=n<400?1:n<1900?0:n<2300?.0001f:.25f;
            const OnePoleFilter *filters[6];for(unsigned v=0;v<6;v++)filters[v]=live[v];
            mixer_audioRouting[voice]=MIXER_ROUTING_FX_BUS;
            float gains[64];const float *g=FxBus_clipFaderGains(mixer_audioRouting,filters,targets,gains,size);assert(g);
            for(unsigned k=0;k<size;k++){
                float actual=calcOnePole(live[voice],targets[voice]);
                assert(actual==g[k] && isfinite(g[k]) && g[k]>=0 && g[k]<=1);
                trajectory[partition][n+k]=g[k];
                if(csv && partition==0)fprintf(csv,"%u,%u,%.9g\n",voice,n+k,g[k]);
            }
            n+=size;
        }
        if(partition)assert(!memcmp(trajectory[0],trajectory[partition],sizeof trajectory[0]));
    }
    if(csv)assert(!fclose(csv));
    /* All six possible sole voices x all six output routes. */
    for(unsigned voice=0;voice<6;voice++)for(unsigned route=0;route<6;route++){
        init();fxBus.audioRouting=route;mixer_audioRouting[voice]=MIXER_ROUTING_FX_BUS;
        live[voice]->b=targets[voice]=0;
        const float *g=mixer_clipFaderForBlock();assert(g);
        _Alignas(4) int16_t input[128]={0},a[128]={0},b[128]={0};
        for(unsigned i=0;i<128;i++)input[i]=5000;
        FxBus_processWithClipGain(input,a,a+1,b,b+1,64,g);
        assert(fxBus.distortion.clipStateActive && fxBus.distortion.clipFeedbackGain==1);
        double history=fxBus.distortion.lastOut[0];assert(history!=0); /* new output zero did not close loop */
        live[voice]->b=targets[voice]=.25;
        g=mixer_clipFaderForBlock();assert(g && g[0]==.25f);
        assert(fxBus.distortion.lastOut[0]==history); /* fader movement requests no reset */
        FxBus_prepareFxChange();assert(!fxBus.distortion.clipStateActive);
        memset(input,0,sizeof input);memset(a,0,sizeof a);memset(b,0,sizeof b);
        FxBus_processWithClipGain(input,a,a+1,b,b+1,64,g);
        for(unsigned i=0;i<128;i++)assert(!a[i] && !b[i]);
        assert(mixer_clipFaderForBlock()[0]==.25f); /* reset did not invent a new fader start */
        mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_FX_BUS;targets[(voice+1)%6]=0;
        assert(!mixer_clipFaderForBlock()); /* silent extra voice still counts; unity fallback */
        mixer_audioRouting[voice]=mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_DAC1_STEREO;
        assert(!mixer_clipFaderForBlock()); /* no stale zero gain */
        mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_FX_BUS;
        live[(voice+1)%6]->b=targets[(voice+1)%6]=.64f;
        assert(mixer_clipFaderForBlock()[0]==.64f); /* new sole voice, not previous association */
    }
    puts("PASS actual capture+replay+bus: all 64 route masks, 36 sole/output routes, live filters untouched, mirrored samples exact, 1/7/64 partition, routing/FX/kit reset transitions");
}
