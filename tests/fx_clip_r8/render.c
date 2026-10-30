/* Render the actual linked production DSP. Input generation is synthetic. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/DSPAudio/FX/Distortion.h"
#define FS 44140
static double kick(double t) {
    return t >= 0 && t < .18 ? exp(-t*24)*sin(6.283185307179586*(60*t+1.2*(1-exp(-t*40)))) : 0;
}
static void finite_state(const MasterDistortion *d) {
    for(unsigned c=0;c<2;c++) {
        assert(isfinite(d->hp1_state[c]) && isfinite(d->hp3_state[c]));
        assert(isfinite(d->lp1_state[c]) && isfinite(d->lp2_state[c]));
        assert(isfinite(d->lp3_state[c]) && isfinite(d->slewState[c]) && isfinite(d->lastOut[c]));
#ifdef R7
        assert(isfinite(d->clipBassLp_state[c]));
#endif
    }
    assert(isfinite(d->clipFeedbackGain));
}
int main(int argc,char **argv) {
    if(argc!=11)return 2;
    unsigned profile=atoi(argv[9]),voice=atoi(argv[10]);assert(profile<5 && voice<6);
    unsigned scenario=atoi(argv[1]),row=atoi(argv[2]),mode=atoi(argv[4]),block=atoi(argv[5]),type=atoi(argv[6]);
    double travel=atof(argv[3]); float dw=atof(argv[7]);
    assert(row<7 && mode<3 && block>0 && block<=64 && type<3 && travel>=0 && travel<=1);
    const int rows[][4]={{63,7,127,127},{63,21,0,63},{32,44,63,32},{127,127,127,127},{63,0,127,127},{63,7,127,0},{0,127,0,127}};
    MasterDistortion d;memset(&d,0,sizeof d);Distortion_init(&d);d.distortionType=type;
    d.drive=rows[row][0]/127.f;d.p1=rows[row][1]/127.f;d.p2=rows[row][2]/127.f;d.p3=rows[row][3]/127.f;
    if(scenario==9){d.clipStateActive=1;d.clipFeedbackGain=1;d.lastOut[0]=.2;d.lastOut[1]=-.17;}
    FILE *f=fopen(argv[8],"wb");if(!f)return 3;
    unsigned last_input=0;
    unsigned long long core_hash=14695981039346656037ULL;
    double max_wet=0;
    float fader=travel*travel;
    double gain_sum=0;unsigned gain_count=0;

    for(unsigned n=0;n<FS*3/2;) {
        unsigned size=FS*3/2-n<block?FS*3/2-n:block;
        struct {int32_t guard; _Alignas(4) int16_t samples[128]; int32_t end;} in,out;
        memset(&in,0,sizeof in);memset(&out,0,sizeof out);
        in.guard=out.guard=0x12345678;in.end=out.end=0x76543210;
        float gains[64];
        for(unsigned i=0;i<size;i++) {
            double t=(n+i)/(double)FS,v=0,w=0,g=travel*travel;
            if(profile==2 || profile==3){
                float p=t<.2?travel:t<.7?travel*(.7-t)/.5:t<.9?0:t<1.2?travel*(t-.9)/.3:travel;
                float target=p*p;
                fader=fmaf(voice<3?.01f:.1f,target-fader,fader);
                if(profile==2)g=fader;
            }
            gains[i]=(profile==2 || profile==3)?fader:profile==1?travel*travel:1;
            gain_sum+=gains[i];gain_count++;
            switch(scenario) {
            case 0:v=kick(t);break;
            case 1:if(t<.5)v=.7*sin(6.283185307179586*60*t);break;
            case 2:if(t<.5)v=.45*sin(6.283185307179586*60*t)+.2*sin(6.283185307179586*1400*t);break;
            case 3:v=kick(t)+kick(t-.22)+kick(t-.70);break;
            case 4:v=exp(-t*24)*sin(6.283185307179586*60*t);break;
            case 5:v=kick(t)+kick(t-.60);break;
            case 6:if(t<.5){double p=travel*(1-t/.5);g=p*p;v=.8*sin(6.283185307179586*60*t);}break;
            case 7:if(t<.5)v=((n+i)&1)?-1:1;break;
            case 8:if(n+i==0)v=-1;break;
            case 9:break;
            case 10:if(t<.5)v=-1;break;
            case 11:if(t<.5)v=1;break;
            case 12:if(t<1.4)v=.7*(kick(t)+kick(t-.55)+kick(t-1.05))+.01*sin(6.283185307179586*1400*t);break;
            default:return 4;
            }
            /* mode 0 mono; 1 identical stereo; 2 asymmetric stereo. */
            w=mode==2 && scenario!=9 ? .31*kick(t-.04) : v;
            in.samples[2*i]=(int16_t)((scenario==10?32768:32767)*g*v);
            in.samples[2*i+1]=(int16_t)(32767*g*w);
            if(in.samples[2*i] || (mode && in.samples[2*i+1]))last_input=n+i+1;
        }
        int16_t original[128];memcpy(original,in.samples,sizeof original);
#ifdef R8
        Distortion_processWithClipGain(&d,in.samples,out.samples,mode?out.samples+1:NULL,size,mode?2:1,dw,profile?gains:NULL);
#else
        Distortion_process(&d,in.samples,out.samples,mode?out.samples+1:NULL,size,mode?2:1,dw);
#endif
        assert(!memcmp(original,in.samples,sizeof original));
        finite_state(&d);
        /* Fingerprint the core return at block boundaries; bass must not enter it. */
        const unsigned char *bits=(const unsigned char*)d.lastOut;
        for(unsigned k=0;k<sizeof d.lastOut;k++){core_hash^=bits[k];core_hash*=1099511628211ULL;}
        for(unsigned c=0;c<(mode?2:1);c++) {
            double wet=d.lastOut[c];
#ifdef R8
            wet*=gains[size-1];
#endif
#ifdef R7
            wet+=d.clipBassLp_state[c]*TEST_BASS_WEIGHT*d.p3;
#endif
            if(fabs(wet)>max_wet)max_wet=fabs(wet);
            assert(fabs(wet)<=.94749); /* Includes the host-only .30 comparison weight. */
        }
        assert(in.guard==0x12345678 && out.guard==0x12345678 && in.end==0x76543210 && out.end==0x76543210);
        if(fwrite(out.samples,sizeof(int16_t),2*size,f)!=2*size)return 5;
        n+=size;
    }
    printf("%u %016llx %.17g %.17g\n",last_input,core_hash,max_wet,gain_sum/gain_count);
    return fclose(f)!=0;
}
