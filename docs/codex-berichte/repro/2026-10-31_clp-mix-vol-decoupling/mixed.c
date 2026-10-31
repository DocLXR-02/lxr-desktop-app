/* Architectural counterexamples, not a firmware candidate. */
#define main solo_fixture_main
#include "experiment.c"
#undef main
int main(int argc,char **argv){
    assert(argc==5);float m[2]={atoi(argv[1])/127.f,atoi(argv[2])/127.f},r=fmaxf(m[0],m[1]);
    unsigned drive=atoi(argv[3]),clips[3]={0};const unsigned kinds[2]={0,5};
    VoiceFixture v[3][2];MasterDistortion d[5];
    for(unsigned p=0;p<3;p++)for(unsigned j=0;j<2;j++)init_voice(&v[p][j],kinds[j],p==0?m[j]:p==1?1:r>0?m[j]/r:0,drive);
    for(unsigned p=0;p<5;p++)init_dsp(d+p);
    FILE *f=fopen(argv[4],"wb");assert(f);
    for(unsigned n=0;n<N;n+=64){
        _Alignas(4) int16_t samples[3][2][64],bus[3][128]={0},single[2][128]={0},out[5][128]={0};
        float level[64],gain[64],t[6]={1,1,1,1,1,1};uint8_t routes[6]={6,0,0,0,0,6};
        const OnePoleFilter *filters[6];for(unsigned k=0;k<6;k++)filters[k]=&v[0][0].FaderVolumeFilter;filters[5]=&v[0][1].FaderVolumeFilter;
        assert(FxBus_clipFaderGains(routes,filters,t,gain,64));
        for(unsigned p=0;p<3;p++)for(unsigned j=0;j<2;j++){
            render_voice(&v[p][j],kinds[j],p==0?m[j]:p==1?1:r>0?m[j]/r:0,1,n,samples[p][j],level);
            if(m[j]==0)memset(samples[p][j],0,sizeof samples[p][j]);
            add_voice(bus[p],samples[p][j],kinds[j],clips+p);
            if(p==1){unsigned ignored=0;add_voice(single[j],samples[p][j],kinds[j],&ignored);}
        }
        for(unsigned p=0;p<5;p++){
            int16_t *in=p<3?bus[p]:single[p-3];
            Distortion_processWithClipGain(d+p,in,out[p],out[p]+1,64,2,1,gain);finite_dsp(d+p);
        }
        if(r==1)assert(!memcmp(d,d+2,sizeof d[0])&&!memcmp(out[0],out[2],sizeof out[0]));
        for(unsigned i=0;i<64;i++){
            float row[16]={bus[0][2*i],bus[0][2*i+1],bus[1][2*i],bus[1][2*i+1],bus[2][2*i],bus[2][2*i+1],out[0][2*i],out[0][2*i+1],out[1][2*i],out[1][2*i+1],out[2][2*i]*r,out[2][2*i+1]*r,out[3][2*i]*m[0]+out[4][2*i]*m[1],out[3][2*i+1]*m[0]+out[4][2*i+1]*m[1],out[1][2*i]*r,out[1][2*i+1]*r};
            for(unsigned k=0;k<16;k++)assert(isfinite(row[k]));assert(fwrite(row,sizeof row,1,f)==1);
        }
    }
    assert(!fclose(f));printf("%u %u %u\n",clips[0],clips[1],clips[2]);
}
