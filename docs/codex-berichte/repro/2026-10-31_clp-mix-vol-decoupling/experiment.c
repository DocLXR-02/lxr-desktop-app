/* Offline reference arrangements. Voice oscillators/filters/envelopes are not simulated. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/DSPAudio/distortion.h"
#define OUTPUT_DMA_SIZE 64
#define FS 44140
#define N (64*966)
typedef struct {float vol,velo,egValueOscVol,combinedVolume,mix;unsigned volumeMod;Distortion distortion;OnePoleFilter FaderVolumeFilter,MixerPageFilter[2];} VoiceFixture;
static VoiceFixture voiceArray[3],snareVoice,cymbalVoice,hatVoice;
static float targets[6];
static float adc_getFaderVolume(unsigned v){return targets[v];}
static uint8_t mixer_checkOutJackAvailable(uint8_t dest){return dest;}
/* Verbatim extracted tails/helpers. Only wrapper signatures are new. */
#include "tails.inc"
static VoiceFixture *slot(unsigned v){return v<3?voiceArray+v:v==3?&snareVoice:v==4?&cymbalVoice:&hatVoice;}
static void init_voice(VoiceFixture *v,unsigned kind,float m,unsigned drive){
    memset(v,0,sizeof *v);v->vol=m;v->velo=.8f;v->egValueOscVol=.8f;v->volumeMod=1;v->mix=.5f;
    v->FaderVolumeFilter.r=kind<3?.01:.1;v->FaderVolumeFilter.b=1;
    v->MixerPageFilter[0].r=.01;v->MixerPageFilter[0].b=m;
    setDistortionShape(&v->distortion,drive/127.f);
    v->distortion.DriveParamFilter.r=.01;v->distortion.DriveParamFilter.b=v->distortion.shape;
}
static float travel(unsigned profile,double t){return profile!=1?1:t<.1?1:t<.6?(.6-t)/.5:t<.8?0:t<1.1?(t-.8)/.3:1;}
static float mix_target(unsigned profile,float m,double t){return profile!=2?m:t<.25?1:t<.5?m:t<.8?0:t<1.15?m:0;}
static double kick(double t){return t>=0 && t<.18?exp(-24*t)*sin(6.283185307179586*(60*t+1.2*(1-exp(-40*t)))):0;}
static double fixture(double t){return t<1.15?.8*(kick(t)+kick(t-.42)+kick(t-.85))+.06*sin(6.283185307179586*1400*t)*((t<.32)||(t>=.85 && t<1.10)):0;}
static void render_voice(VoiceFixture *v,unsigned kind,float m,float q,unsigned n,int16_t out[64],float mixgain[64]){
    *slot(kind)=*v;slot(kind)->vol=m;targets[kind]=q;
    OnePoleFilter replay=v->MixerPageFilter[0];
    for(unsigned i=0;i<64;i++)mixgain[i]=kind<3?calcOnePole(&replay,m):m;
    _Alignas(4) int16_t trans[64],osc[64];
    for(unsigned i=0;i<64;i++){
        double s=fixture((n+i)/(double)FS);
        out[i]=(int16_t)(32767*.75*s*(kind<3?.8:1));
        trans[i]=(int16_t)(32767*.02*s);osc[i]=(int16_t)(32767*.25*s);
    }
    if(kind<3)drum_tail(kind,out,64);
    else if(kind==3)snare_tail(out,osc,trans,64);
    else if(kind==4)cymbal_tail(out,trans,64);
    else hat_tail(out,trans,64);
    *v=*slot(kind);
    if(kind<3)assert(v->MixerPageFilter[0].b==replay.b);
}
static void add_voice(int16_t bus[128],int16_t samples[64],unsigned kind,unsigned *clips){
    float pan=kind<3?.3f:.7f,pl=sqrtf(1-pan),pr=sqrtf(pan);
    for(unsigned i=0;i<64;i++)for(unsigned c=0;c<2;c++){
        int x=bus[2*i+c]+(int16_t)(samples[i]*(c?pr:pl));*clips+=(x>32767||x< -32768);
    }
    _Alignas(4) int16_t a[128]={0},b[128]={0};
    mixer_addDataToOutput(MIXER_ROUTING_FX_BUS,pl,pr,samples,a,a+1,b,b+1,bus);
}
static void init_dsp(MasterDistortion *d){memset(d,0,sizeof *d);Distortion_init(d);d->distortionType=DISTORTION_RAT;d->drive=63/127.f;d->p1=7/127.f;d->p2=d->p3=1;}
static void finite_dsp(const MasterDistortion *d){for(unsigned c=0;c<2;c++)assert(isfinite(d->lastOut[c])&&isfinite(d->clipBassLp_state[c])&&isfinite(d->hp1_state[c])&&isfinite(d->lp1_state[c])&&isfinite(d->hp3_state[c])&&isfinite(d->lp2_state[c])&&isfinite(d->lp3_state[c])&&isfinite(d->slewState[c]));assert(isfinite(d->clipFeedbackGain));}
static void gains(VoiceFixture *v,unsigned kind,float q,float g[64]){
    uint8_t routes[6]={0};routes[kind]=MIXER_ROUTING_FX_BUS;
    const OnePoleFilter *p[6];for(unsigned k=0;k<6;k++)p[k]=&v->FaderVolumeFilter;
    float t[6];for(unsigned k=0;k<6;k++)t[k]=q;
    VoiceFixture before=*v;assert(FxBus_clipFaderGains(routes,p,t,g,64)==g);assert(!memcmp(&before,v,sizeof before));
}
int main(int argc,char **argv){
    assert(argc==7 || argc==8);float dw=argc==8?atof(argv[7]):1;unsigned kind=atoi(argv[1]),drive=atoi(argv[3]),profile=atoi(argv[4]);float m=atoi(argv[2])/127.f;
    assert(kind<6 && drive<=127 && profile<=2);
    FILE *file=fopen(argv[5],"wb"),*states=fopen(argv[6],"wb");assert(file&&states);
    VoiceFixture voice[2];init_voice(voice,kind,profile==2?1:m,drive);init_voice(voice+1,kind,1,drive);
    MasterDistortion d[2];init_dsp(d);init_dsp(d+1);unsigned clips=0;
    for(unsigned n=0;n<N;n+=64){
        double t=n/(double)FS;float u=travel(profile,t),q=u*u,mt=mix_target(profile,m,t);
        float g[2][64],level[2][64];_Alignas(4) int16_t samples[2][64],bus[2][128]={{0}},out[2][128]={{0}};
        for(unsigned p=0;p<2;p++){
            gains(voice+p,kind,q,g[p]);render_voice(voice+p,kind,p?1:mt,q,n,samples[p],level[p]);
            if(p && mt==0)memset(samples[p],0,sizeof samples[p]); /* explicit zero: no hidden CLP excitation */
            add_voice(bus[p],samples[p],kind,&clips);
            Distortion_processWithClipGain(d+p,bus[p],out[p],out[p]+1,64,2,dw,g[p]);finite_dsp(d+p);
        }
        assert(!memcmp(g[0],g[1],sizeof g[0]));
        if(profile!=2 && m==1)assert(!memcmp(out[0],out[1],sizeof out[0]) && !memcmp(d,d+1,sizeof d[0]));
        for(unsigned i=0;i<64;i++){
            float post=mt==0?0:level[0][i];
            float row[15]={bus[0][2*i],bus[0][2*i+1],bus[1][2*i],bus[1][2*i+1],out[0][2*i],out[0][2*i+1],out[1][2*i],out[1][2*i+1],(int16_t)(out[1][2*i]*post),(int16_t)(out[1][2*i+1]*post),post,g[0][i],d[0].lastOut[0],d[1].lastOut[0],mt};
            if(mt==0)assert(!row[2]&&!row[3]&&!row[8]&&!row[9]);
            assert(fwrite(row,sizeof row,1,file)==1);
        }
        assert(fwrite(d,sizeof d,1,states)==1);
    }
    printf("%u %zu\n",clips,sizeof d[0]);assert(!fclose(file)&&!fclose(states));
}
