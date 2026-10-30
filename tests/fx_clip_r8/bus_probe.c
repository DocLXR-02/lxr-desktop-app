#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define main inherited_bus_main
#include "../fx_clip/fxbus.c"
#undef main
int main(int argc,char **argv){
    assert(argc==5);unsigned mask=atoi(argv[1]),route=atoi(argv[2]);float level=atof(argv[3]);
    FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.audioRouting=route;fxBus.dryWet=.4f;
    fxBus.distortion.distortionType=DISTORTION_RAT;
    fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=fxBus.distortion.p3=1;
    uint8_t routes[6];OnePoleFilter filters[6];const OnePoleFilter *pointers[6];float targets[6];
    for(unsigned v=0;v<6;v++){
        routes[v]=(mask&(1u<<v))?MIXER_ROUTING_FX_BUS:MIXER_ROUTING_DAC1_STEREO;
        targets[v]=level;
        if(mask==3 && v==1)targets[v]=0; /* additional silent routed voice */
        filters[v].r=v<3?.01:.1;filters[v].b=targets[v];pointers[v]=&filters[v];
    }
    FILE *file=fopen(argv[4],"wb");assert(file);
    for(unsigned n=0;n<1024;n+=64){
        _Alignas(4) int16_t in[128],a[128]={0},b[128]={0};
        for(unsigned i=0;i<64;i++){in[2*i]=12000*sin((n+i)*.00854);in[2*i+1]=4000*cos((n+i)*.012);}
#ifdef R8
        float gains[64];const float *g=FxBus_clipFaderGains(routes,pointers,targets,gains,64);
        FxBus_processWithClipGain(in,a,a+1,b,b+1,64,g);
#else
        FxBus_process(in,a,a+1,b,b+1,64);
#endif
        assert(fwrite(a,sizeof a,1,file)==1 && fwrite(b,sizeof b,1,file)==1);
    }
    return fclose(file)!=0;
}
