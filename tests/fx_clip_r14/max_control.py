#!/usr/bin/env python3
"""Host-check the actual source-extracted R14 MAX selector, all 64 routing masks."""
from pathlib import Path
import os, subprocess, tempfile
root = Path(__file__).resolve().parents[2]
src = (root/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text()
beg = '/* R14: select the highest *existing* smoothed fader among FX routes.'
end = 'IN_ITCM_RAM void mixer_calcNextSampleBlock('
assert src.count(beg) == src.count(end) == 1
helper = src[src.index(beg):src.index(end)]
assert 'noinline' in helper and 'sqrtf(maxQ)' in helper
assert src.count('mixer_clipMaxRoutedFaderGain()') == 2
assert 'clpFader' not in src
header = r"""
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef struct { float b; } OnePoleFilter;
typedef struct { OnePoleFilter FaderVolumeFilter; } Voice;
static uint8_t mixer_audioRouting[6];
static Voice voiceArray[3], snareVoice, cymbalVoice, hatVoice;
#define MIXER_ROUTING_FX_BUS 7
"""
body = r"""
static void setQ(int i,float q) {
    switch(i) {
        case 0: voiceArray[0].FaderVolumeFilter.b=q;break;
        case 1: voiceArray[1].FaderVolumeFilter.b=q;break;
        case 2: voiceArray[2].FaderVolumeFilter.b=q;break;
        case 3: snareVoice.FaderVolumeFilter.b=q;break;
        case 4: cymbalVoice.FaderVolumeFilter.b=q;break;
        case 5: hatVoice.FaderVolumeFilter.b=q;break;
    }
}
static float r13(float q) {
    if(q<=0.f) return 0.f;
    if(q>=.25f) return 1.f;
    return 2.f*sqrtf(q);
}
static uint32_t seed=0x72a741u;
static uint32_t rng(void) { seed=seed*1664525u+1013904223u; return seed; }
int main(void) {
    const float values[]={0.f,.0001f,.0025f,.01f,.0625f,.2499f,.25f,.5f,1.f};
    int cases=0,single=0,multi=0,zero=0;
    for(unsigned mask=0;mask<64;mask++) {
        for(unsigned trial=0;trial<256;trial++) {
            float maxQ=-1.f;
            for(int i=0;i<6;i++) {
                float u=(trial<9)?values[(trial+i)%9]:(float)(rng()%10001u)/10000.f;
                float q=(trial<9)?u:u*u;
                setQ(i,q);
                mixer_audioRouting[i]=(mask&(1u<<i))?MIXER_ROUTING_FX_BUS:0;
                if((mask&(1u<<i)) && q>maxQ)maxQ=q;
            }
            float expected=maxQ<0.f?1.f:r13(maxQ);
            float actual=mixer_clipMaxRoutedFaderGain();
            if(memcmp(&expected,&actual,sizeof(float))) {
                fprintf(stderr,"R14 MAX FAIL mask=%u trial=%u expected=%g actual=%g\n",
                        mask,trial,expected,actual);
                return 1;
            }
            if(mask==0)zero++;
            else if((mask&(mask-1))==0)single++;
            else multi++;
            cases++;
        }
    }
    for(unsigned i=0;i<4096;i++) {
        float a=(float)(rng()%10001u)/10000.f;
        float b=(float)(rng()%10001u)/10000.f;
        float step=(b-a)/63.f;
        if(fabsf(a+63.f*step-b)>1e-6f)return 2;
    }
    printf("R14 source-extracted MAX PASS: %d cases (zero %d, singleton %d, multi %d); 4096 ramp endpoints PASS\n",
           cases,zero,single,multi);
    return 0;
}
"""
with tempfile.TemporaryDirectory() as tmp:
    srcfile=Path(tmp)/'selector.c'
    exe=Path(tmp)/'selector'
    srcfile.write_text(header+helper+body)
    cmd=[os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror',
         '-fsanitize=undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all',
         str(srcfile),'-lm','-o',str(exe)]
    print('Compiler:',' '.join(cmd),flush=True)
    subprocess.run(cmd,check=True)
    subprocess.run([str(exe)],check=True)
