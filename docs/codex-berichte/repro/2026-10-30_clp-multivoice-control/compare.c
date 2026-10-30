/* Offline policy comparison only. No firmware implementation. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#define OUTPUT_DMA_SIZE 64
#define FS 44140
#define N (64*1104)
/* Actual pinned mapper and mixer adder, extracted by compare.py. */
static uint8_t mixer_checkOutJackAvailable(uint8_t dest){return dest;}
#include "extracted.inc"
static float gains[3][N];
static int16_t input[N*2],audio[3][N*2];
static double kick(double t){return t>=0 && t<.16 ? exp(-24*t)*sin(6.283185307179586*(60*t+1.2*(1-exp(-40*t)))):0;}
static float fade(double t){return t<.2?1:t<.7?(.7-t)/.5:t<.9?0:t<1.2?(t-.9)/.3:1;}
static unsigned controls(unsigned sc,double t,float u[6]){
    memset(u,0,6*sizeof *u);
    switch(sc){
    case 0:u[0]=fade(t);return 1; /* sole */
    case 1:u[0]=.4;u[1]=.2*fade(t);return 3; /* fade lower */
    case 2:u[0]=.4*fade(t);u[1]=.2;return 3; /* fade higher */
    case 3:u[0]=.8;u[1]=.2*fade(t);return 3; /* silent high */
    case 4:u[1]=.2*fade(t);return 3; /* same audio, silent zero */
    case 5:u[0]=.1;u[1]=.2*fade(t);return 3; /* same audio, silent low */
    case 6:u[0]=.4*fade(t);u[1]=.25*fade(t);return 3; /* collective */
    case 7:u[0]=.2;u[1]=.15;return 3; /* sparse/alternating */
    case 8:for(unsigned v=0;v<6;v++)u[v]=(.3-.05*v)*fade(t);return 63;
    case 9:u[0]=.2;u[1]=t<.55?0:.4;return t<.3?1:t<.8?3:t<1?1:t<1.1?0:1; /* routes */
    case 10:for(unsigned v=0;v<6;v++)u[v]=fade(t);return 63; /* hot sum */
    default:abort();
    }
}
static double source(unsigned sc,unsigned v,double t){
    if(t>=1.35)return 0;
    if((sc>=3 && sc<=5 && v==0)||(sc==9 && v==1))return 0;
    if(sc==7){
        double value=0;
        for(unsigned k=0;k<8;k++)if(k%2==v)value+=kick(t-.02-.16*k);
        return .7*value;
    }
    if(sc==10)return .65*sin(6.283185307179586*80*t);
    return .55*(kick(t-.02-.025*v)+kick(t-.51-.01*v)+kick(t-1.05-.015*v))
           +.008*sin(6.283185307179586*(1400+137*v)*t);
}
static void finite_state(const MasterDistortion *d){
    for(unsigned c=0;c<2;c++)assert(isfinite(d->hp1_state[c]) && isfinite(d->hp2_state[c]) && isfinite(d->hp3_state[c]) && isfinite(d->lp1_state[c]) && isfinite(d->lp2_state[c]) && isfinite(d->lp3_state[c]) && isfinite(d->slewState[c]) && isfinite(d->lastOut[c]) && isfinite(d->clipBassLp_state[c]));
    assert(isfinite(d->clipFeedbackGain));
}
static void init(MasterDistortion *d,unsigned row){
    memset(d,0,sizeof *d);Distortion_init(d);d->distortionType=DISTORTION_RAT;
    d->drive=row?1:63/127.f;d->p1=row?1:7/127.f;d->p2=d->p3=1;
}
int main(int argc,char **argv){
    assert(argc==6);unsigned sc=atoi(argv[1]),row=atoi(argv[2]),channels=atoi(argv[3]);float dw=atof(argv[4]);
    OnePoleFilter live[6];const OnePoleFilter *ptr[6];unsigned last_input=0,clips=0;
    for(unsigned v=0;v<6;v++){live[v].r=v<3?.01:.1;live[v].b=0;ptr[v]=&live[v];}
    MasterDistortion d[3];for(unsigned p=0;p<3;p++)init(&d[p],row);
    FILE *file=fopen(argv[5],"wb");assert(file);
    for(unsigned n=0;n<N;n+=64){
        float u[6],targets[6],q[6][64],r9[64];uint8_t routes[6];
        unsigned mask=controls(sc,n/(double)FS,u),count=0;
        for(unsigned v=0;v<6;v++){targets[v]=u[v]*u[v];routes[v]=(mask>>v)&1?MIXER_ROUTING_FX_BUS:0;count+=(mask>>v)&1;}
        OnePoleFilter before[6];memcpy(before,live,sizeof live);
        const float *old=FxBus_clipFaderGains(routes,ptr,targets,r9,64);
        for(unsigned v=0;v<6;v++){
            OnePoleFilter replay=live[v];for(unsigned i=0;i<64;i++)q[v][i]=calcOnePole(&replay,targets[v]);
        }
        assert(!memcmp(before,live,sizeof live));
        for(unsigned i=0;i<64;i++){
            float maximum=0,sum=0;
            for(unsigned v=0;v<6;v++)if((mask>>v)&1){float bounded=fminf(1,fmaxf(0,q[v][i]));maximum=fmaxf(maximum,bounded);sum+=bounded;}
            gains[0][n+i]=old?old[i]:1;
            gains[1][n+i]=count?FxBus_clipCoreGain(maximum):1;
            gains[2][n+i]=count?FxBus_clipCoreGain(fminf(1,sum)):1;
            assert(gains[1][n+i]>=0 && gains[1][n+i]<=gains[2][n+i] && gains[2][n+i]<=1);
            if(count==1)assert(gains[0][n+i]==gains[1][n+i] && gains[0][n+i]==gains[2][n+i]);
        }
        _Alignas(4) int16_t bus[128]={0},a[128]={0},b[128]={0};
        for(unsigned v=0;v<6;v++){
            int16_t samples[64];float pan=(v+.5f)/6,pl=sqrtf(1-pan),pr=sqrtf(pan);
            for(unsigned i=0;i<64;i++){
                float actual=calcOnePole(&live[v],targets[v]);assert(actual==q[v][i]);
                samples[i]=(int16_t)(32767*actual*source(sc,v,(n+i)/(double)FS));
                if((mask>>v)&1)for(unsigned c=0;c<2;c++){
                    int total=bus[2*i+c]+(int16_t)(samples[i]*(c?pr:pl));
                    clips+=(total>32767 || total< -32768);
                }
            }
            mixer_addDataToOutput(routes[v],pl,pr,samples,a,a+1,b,b+1,bus);
        }
        memcpy(input+2*n,bus,sizeof bus);
        for(unsigned i=0;i<64;i++){
            if(bus[2*i]||(channels==2 && bus[2*i+1]))last_input=n+i+1;
            for(unsigned p=0;p<3;p++){
                int16_t out[2]={0};Distortion_processWithClipGain(&d[p],bus+2*i,out,channels==2?out+1:NULL,1,channels,dw,&gains[p][n+i]);
                audio[p][2*(n+i)]=out[0];audio[p][2*(n+i)+1]=out[1];finite_state(&d[p]);
            }
            assert(!memcmp(&d[0],&d[1],sizeof d[0]) && !memcmp(&d[0],&d[2],sizeof d[0]));
            if(sc==0 || dw==0)assert(!memcmp(audio[0]+2*(n+i),audio[1]+2*(n+i),4) && !memcmp(audio[0]+2*(n+i),audio[2]+2*(n+i),4));
            float record[18]={gains[0][n+i],gains[1][n+i],gains[2][n+i],d[0].lastOut[0],d[0].lastOut[1],d[0].clipBassLp_state[0]*(-.25)*d[0].p3,d[0].clipBassLp_state[1]*(-.25)*d[0].p3,audio[0][2*(n+i)],audio[0][2*(n+i)+1],audio[1][2*(n+i)],audio[1][2*(n+i)+1],audio[2][2*(n+i)],audio[2][2*(n+i)+1],d[0].clipFeedbackGain,bus[2*i],bus[2*i+1],mask,clips};
            assert(fwrite(record,sizeof record,1,file)==1);
        }
    }
    assert(!fclose(file));
    /* Repartition only the DSP: captured metadata/input are the same sequence. */
    for(unsigned p=0;p<3;p++)for(unsigned block=7;block<=64;block=64){
        MasterDistortion replay;init(&replay,row);
        for(unsigned n=0;n<N;){unsigned size=N-n<block?N-n:block;int16_t out[128]={0};
            Distortion_processWithClipGain(&replay,input+2*n,out,channels==2?out+1:NULL,size,channels,dw,gains[p]+n);
            assert(!memcmp(out,audio[p]+2*n,4*size));n+=size;
        }
        assert(!memcmp(&replay,&d[p],sizeof replay));if(block==64)break;
    }
    printf("%u %u %zu\n",last_input,clips,sizeof(MasterDistortion));
}
