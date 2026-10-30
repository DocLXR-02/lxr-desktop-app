#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include DISTORTION_SOURCE
static void init(MasterDistortion *d,unsigned kind){
    memset(d,0,sizeof *d);Distortion_init(d);d->distortionType=kind;
    d->drive=d->p1=d->p2=d->p3=1;
}
static int16_t mixed(int16_t dry,double core,double bass,float gain,float dw){
    int16_t wet=__SSAT((int32_t)((core*gain+bass)*32768.0f),16);
    return dry+__SSAT((wet-dry)*dw,16);
}
int main(void){
    unsigned checks=0;
    for(unsigned kind=0;kind<3;kind++)for(unsigned ch=1;ch<=2;ch++)for(unsigned mix=0;mix<3;mix++){
        float dw=mix==0?0:mix==1?.4f:1;
        MasterDistortion full,scaled;init(&full,kind);scaled=full;
        for(unsigned n=0;n<12000;n++){
            _Alignas(4) int16_t input[2]={(int16_t)(12000*sin(.00854*n)),(int16_t)(9000*cos(.012*n))};
            _Alignas(4) int16_t a[2]={0},b[2]={0};
            float g=n<2000?1:n<4000?.25f:n<6000?0:n<8000?.000001f:.7f;
            Distortion_process(&full,input,a,ch==2?a+1:NULL,1,ch,dw);
            Distortion_processWithClipGain(&scaled,input,b,ch==2?b+1:NULL,1,ch,dw,&g);
            assert(!memcmp(&full,&scaled,sizeof full)); /* all internal histories identical */
            if(kind!=DISTORTION_RAT || dw==0 || g==1)assert(!memcmp(a,b,sizeof a));
            if(kind==DISTORTION_RAT)for(unsigned c=0;c<ch;c++){
                double bass=scaled.clipBassLp_state[c]*CLIP_BASS_WEIGHT*scaled.p3;
                assert(b[c]==mixed(input[c],scaled.lastOut[c],bass,g,dw));
                assert(fabs(scaled.lastOut[c]*g+bass)<.89749);
            }
            checks++;
        }
        /* Zero CLP vol has identical baseline integer-mix behavior for every new gain. */
        full.p3=scaled.p3=0;
        _Alignas(4) int16_t in[2]={-32768,32767},a[2]={0},b[2]={0};float zero=0;
        Distortion_process(&full,in,a,ch==2?a+1:NULL,1,ch,dw);
        Distortion_processWithClipGain(&scaled,in,b,ch==2?b+1:NULL,1,ch,dw,&zero);
        assert(!memcmp(a,b,sizeof a) && !memcmp(&full,&scaled,sizeof full));
    }
    printf("PASS %u identical-input DSP steps: gain never changes histories/bass/dry; unity/zero VOL/DW; Tube/Fold; finite bounded wet\n",checks);
}
