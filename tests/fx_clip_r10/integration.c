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
#include "reference.inc"
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
/* Independent high-precision oracle; all measured gains come from production. */
static float expected(float q){return q<=0?0:q>=.25f?1:(float)(2*sqrt((double)q));}
static float mapped(float q){
    uint8_t routes[6]={MIXER_ROUTING_FX_BUS,0,0,0,0,0};
    OnePoleFilter filter={0};filter.b=q;filter.r=0;
    const OnePoleFilter *filters[6]={&filter,&filter,&filter,&filter,&filter,&filter};
    float target[6]={q,q,q,q,q,q},g;
    assert(FxBus_clipFaderGains(routes,filters,target,&g,1)==&g);return g;
}
static void mapping(void){
    const float q[]={0,.01f,.0625f,.16f,.25f,1};
    const float want[]={0,.2f,.5f,.8f,1,1};
    for(unsigned i=0;i<6;i++)assert(fabsf(mapped(q[i])-want[i])<1e-7f);
    assert(mapped(.5625f)==1 && mapped(-1)==0 && mapped(2)==1);
    assert(mapped(nextafterf(0.f,1.f))>0);
    float below=mapped(nextafterf(.25f,0.f));
    assert(below<=1 && 1-below<2e-7f && mapped(nextafterf(.25f,1.f))==1);
    float previous=0;
    for(unsigned i=0;i<=100000;i++){
        float x=i/100000.f,g=mapped(x);
        assert(isfinite(g) && g>=previous && g>=0 && g<=1);
        assert(fabsf(g-expected(x))<1.2e-7f);previous=g;
    }
    puts("PASS production mapping: physical table, 100001-point monotonic/bounds sweep, knee continuity, negative/over-unity clamps and smallest positive float");
}
static void curve_histories(void){
    unsigned checks=0;
    for(unsigned voice=0;voice<6;voice++)for(unsigned channels=1;channels<=2;channels++){
        init();mixer_audioRouting[voice]=MIXER_ROUTING_FX_BUS;
        MasterDistortion r8=fxBus.distortion,r9=r8;
        for(unsigned n=0;n<12000;n++){
            targets[voice]=n<2000?1:n<4000?.16f:n<6000?0:n<8000?1e-8f:n<10000?.0625f:1;
            const OnePoleFilter *filters[6];for(unsigned v=0;v<6;v++)filters[v]=live[v];
            float gain,reference;assert(FxBus_clipFaderGains(mixer_audioRouting,filters,targets,&gain,1));
            assert(R9_clipFaderGains(mixer_audioRouting,filters,targets,&reference,1));assert(gain==reference);
            calcOnePole(live[voice],targets[voice]);
            _Alignas(4) int16_t in[2]={(int16_t)(12000*sin(.00854*n)),(int16_t)(3000*cos(.012*n))};
            _Alignas(4) int16_t a[2]={0},b[2]={0};
            Distortion_processWithClipGain(&r8,in,a,channels==2?a+1:NULL,1,channels,1,&reference);
            Distortion_processWithClipGain(&r9,in,b,channels==2?b+1:NULL,1,channels,1,&gain);
            assert(!memcmp(&r8,&r9,sizeof r8) && !memcmp(a,b,sizeof a));
            checks++;
        }
    }
    printf("PASS %u per-frame complete DSP memcmp checks with actual R10 vs pinned R9 solo gain/audio, all six replay rates and mono/stereo\n",checks);
}
static void settled(void){
    const float travel[][6]={{.25f,.25f,0,0,0,0},{.25f,.25f,.25f,.25f,.25f,.25f},{.4f,.2f,0,0,0,0},{.2f,.8f,0,0,0,0},{.2f,0,0,0,0,0}};
    const unsigned masks[]={3,63,3,3,3};const float want[]={.5f,.5f,.8f,1,.4f};
    for(unsigned c=0;c<5;c++){
        init();for(unsigned v=0;v<6;v++){
            live[v]->b=targets[v]=travel[c][v]*travel[c][v];
            mixer_audioRouting[v]=(masks[c]>>v)&1?MIXER_ROUTING_FX_BUS:0;
        }
        const float *g=mixer_clipFaderForBlock();assert(g);
        for(unsigned n=0;n<64;n++)assert(fabsf(g[n]-want[c])<1e-7f);
    }
    puts("PASS settled physical faders: two/six .25 -> .5, .4/.2 -> .8, .2/silent .8 -> 1, .2/silent zero -> .4");
}
static float trajectory[3][4096];
int main(int argc,char **argv) {
    FILE *csv=argc>1?fopen(argv[1],"w"):NULL;
    if(argc>1)assert(csv);
    if(csv)fputs("route_mask,frame,qmax,gain\n",csv);
    mapping();settled();curve_histories();inherited_bus_main();init();
    /* Every route mask x every bus destination: actual capture, dispatch and DSP. */
    for(unsigned mask=0;mask<64;mask++)for(unsigned route=0;route<6;route++){
        init();fxBus.audioRouting=route;unsigned count=0;
        for(unsigned v=0;v<6;v++){
            mixer_audioRouting[v]=(mask&(1u<<v))?MIXER_ROUTING_FX_BUS:MIXER_ROUTING_DAC1_STEREO;
            count+=(mask>>v)&1;
        }
        OnePoleFilter before[6],reference[6];
        for(unsigned v=0;v<6;v++)before[v]=reference[v]=*live[v];
        const float *g=mixer_clipFaderForBlock();assert((g!=NULL)==(count>0));
        for(unsigned v=0;v<6;v++)assert(!memcmp(&before[v],live[v],sizeof before[v]));
        for(unsigned n=0;n<64;n++){
            float maximum=0;
            for(unsigned v=0;v<6;v++){
                float q=calcOnePole(&reference[v],targets[v]);
                if((mask>>v)&1)maximum=fmaxf(maximum,fminf(1,fmaxf(0,q)));
            }
            if(g)assert(fabsf(g[n]-expected(maximum))<1.2e-7f);
        }
        _Alignas(4) int16_t input[128],out[256]={0},want[256]={0};
        for(unsigned n=0;n<128;n++)input[n]=(int16_t)(12000*sin(n*.07));
        MasterDistortion ref=fxBus.distortion;
        const unsigned dest[]={128,0,128,129,0,1};unsigned ch=route<2?2:1;
        Distortion_processWithClipGain(&ref,input,want+dest[route],ch==2?want+dest[route]+1:NULL,64,ch,1,g);
        FxBus_processWithClipGain(input,out,out+1,out+128,out+129,64,g);
        assert(!memcmp(out,want,sizeof out) && !memcmp(&ref,&fxBus.distortion,sizeof ref));
    }
    for(unsigned other=0;other<=4;other++)if(other!=FX_DISTORTION){fxBus.fxType=other;assert(!mixer_clipFaderForBlock());}
    fxBus.fxType=FX_DISTORTION;
    for(unsigned kind=0;kind<2;kind++){fxBus.distortion.distortionType=kind;assert(!mixer_clipFaderForBlock());}
    /* All masks, both live rates, target events and maximum-dominance crossovers. */
    for(unsigned mask=0;mask<64;mask++)for(unsigned partition=0;partition<3;partition++){
        init();unsigned block=partition==0?1:partition==1?7:64;
        for(unsigned v=0;v<6;v++)mixer_audioRouting[v]=(mask>>v)&1?MIXER_ROUTING_FX_BUS:0;
        for(unsigned n=0;n<4096;){
            unsigned end=n<400?400:n<1900?1900:n<2300?2300:4096,size=end-n<block?end-n:block;
            for(unsigned v=0;v<6;v++)targets[v]=n<400?(v%2?.16f:.0625f):n<1900?(v<3?0:.04f):n<2300?(v%2?1e-8f:.0001f):.25f*(v+1)/6;
            const OnePoleFilter *filters[6];OnePoleFilter before[6];
            for(unsigned v=0;v<6;v++){filters[v]=live[v];before[v]=*live[v];}
            struct {float before;float samples[64];float after;} buffer={123,{0},456};
            const float *g=FxBus_clipFaderGains(mixer_audioRouting,filters,targets,buffer.samples,size);
            assert((g!=NULL)==(mask!=0) && buffer.before==123 && buffer.after==456);
            for(unsigned v=0;v<6;v++)assert(!memcmp(&before[v],live[v],sizeof before[v]));
            float old[64];const float *prior=R9_clipFaderGains(mixer_audioRouting,filters,targets,old,size);
            for(unsigned k=0;k<size;k++){
                float maximum=0;
                for(unsigned v=0;v<6;v++){
                    float q=calcOnePole(live[v],targets[v]);
                    if((mask>>v)&1)maximum=fmaxf(maximum,fminf(1,fmaxf(0,q)));
                }
                float actual=g?g[k]:1;
                assert(isfinite(actual) && actual>=0 && actual<=1);
                assert(fabsf(actual-(mask?expected(maximum):1))<1.2e-7f);
                if(prior)assert(actual==prior[k]);
                trajectory[partition][n+k]=actual;
                if(csv && partition==0)fprintf(csv,"%u,%u,%.9g,%.9g\n",mask,n+k,maximum,actual);
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
        g=mixer_clipFaderForBlock();assert(g && g[0]==1.f);
        assert(fxBus.distortion.lastOut[0]==history); /* fader movement requests no reset */
        FxBus_prepareFxChange();assert(!fxBus.distortion.clipStateActive);
        memset(input,0,sizeof input);memset(a,0,sizeof a);memset(b,0,sizeof b);
        FxBus_processWithClipGain(input,a,a+1,b,b+1,64,g);
        for(unsigned i=0;i<128;i++)assert(!a[i] && !b[i]);
        assert(mixer_clipFaderForBlock()[0]==1.f); /* reset did not invent a new fader start */
        mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_FX_BUS;targets[(voice+1)%6]=0;
        assert(mixer_clipFaderForBlock()[0]==1.f); /* silent extra voice counted; sole high remains high */
        live[voice]->b=targets[voice]=.04f;live[(voice+1)%6]->b=targets[(voice+1)%6]=0;
        assert(fabsf(mixer_clipFaderForBlock()[0]-.4f)<1e-7f); /* zero extra does not force unity */
        live[(voice+1)%6]->b=targets[(voice+1)%6]=.64f;
        assert(mixer_clipFaderForBlock()[0]==1.f); /* high silent extra holds open */
        mixer_audioRouting[voice]=mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_DAC1_STEREO;
        assert(!mixer_clipFaderForBlock()); /* no stale zero gain */
        mixer_audioRouting[(voice+1)%6]=MIXER_ROUTING_FX_BUS;
        live[(voice+1)%6]->b=targets[(voice+1)%6]=.64f;
        assert(mixer_clipFaderForBlock()[0]==1.f); /* new sole voice, not previous association */
    }
    puts("PASS actual capture+replay+bus: all 64 masks x six output destinations, 36 sole/output lifecycle routes, live filters untouched, mapped replay samples verified, 1/7/64 partition, routing/FX/kit reset transitions");
}
