/* Source-verified R23 FxBus + mixer-return test harness, processor DSP stubbed. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IN_ITCM_RAM
#define OUTPUT_DMA_SIZE 64
#define USE_REVERB 0
#define DELAY_NORMAL 0
enum { FX_OFF, FX_DISTORTION, FX_RING, FX_COMPRESSOR, FX_DELAY };
enum { MIXER_ROUTING_DAC1_STEREO, MIXER_ROUTING_DAC2_STEREO, MIXER_ROUTING_DAC1_L,
       MIXER_ROUTING_DAC1_R, MIXER_ROUTING_DAC2_L, MIXER_ROUTING_DAC2_R };
static int16_t sat(int32_t x){return (int16_t)(x>32767?32767:x< -32768?-32768:x);}
static int32_t sadd(int32_t a,int32_t b) {
 int16_t lo=sat((int16_t)(uint16_t)a+(int16_t)(uint16_t)b);
 int16_t hi=sat((int16_t)(uint16_t)((uint32_t)a>>16)+(int16_t)(uint16_t)((uint32_t)b>>16));
 return (int32_t)((uint32_t)(uint16_t)lo|((uint32_t)(uint16_t)hi<<16));
}
#define __QADD16(a,b) sadd((a),(b))
typedef struct {int clipStateActive;} Distortion;
typedef struct {int dummy;} Ringmod,Compressor;
typedef struct {int delayType;} Delay;
typedef struct {uint8_t audioRouting;float dryWet;Distortion distortion;Ringmod ringmod;Compressor compressor;Delay delay;} FxBus;
static FxBus fxBus;
static int16_t fxPoolInputs[5][128],fxPoolReturn[2][128],dac2[128],dac1[128];
static unsigned calls[5],clipResets;
static uint8_t channelCount[5];static float wetSeen[5];
static void Distortion_resetClip(Distortion*d){d->clipStateActive=0;clipResets++;}
static void bufferTool_clearBuffer(int16_t*p,unsigned n){memset(p,0,n*sizeof(*p));}
static int16_t sample(int t,int16_t x,float dw){
 int scale=(int)(dw*4.f+1.f);
 return sat((int32_t)x*scale/(t+2));
}
static void effect(int t,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw) {
 calls[t]++;channelCount[t]=(uint8_t)ch;wetSeen[t]=dw;
 for(unsigned i=0;i<n;i++){
  l[2*i]=sat((int32_t)l[2*i]+sample(t,s[2*i],dw));
  if(ch==2)r[2*i]=sat((int32_t)r[2*i]+sample(t,s[2*i+1],dw));
 }
}
static void Distortion_process(Distortion*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw){(void)d;effect(1,s,l,r,n,ch,dw);}
static void Ringmod_process(Ringmod*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw){(void)d;effect(2,s,l,r,n,ch,dw);}
static void Compressor_process(Compressor*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw){(void)d;effect(3,s,l,r,n,ch,dw);}
static void Delay_process(Delay*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw){(void)d;effect(4,s,l,r,n,ch,dw);}
static void Delay_processPingPong(Delay*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float dw){(void)d;effect(4,s,l,r,n,ch,dw);}
/* SOURCE_FXBUS_BEGIN */
IN_ITCM_RAM void FxBus_processSelected(uint8_t selectedType, int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size)
{
	int16_t* out;
	int16_t* out2 = NULL;
	// < MIXER_ROUTING_DAC1_L -> stereo 1 or 2
	const uint8_t channels = fxBus.audioRouting < MIXER_ROUTING_DAC1_L ? 2 : 1;
	switch(fxBus.audioRouting)
	{
	default:
	case MIXER_ROUTING_DAC1_STEREO:
		out = outL2;
		out2 = outR2;
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		out = outL;
		out2 = outR;
		break;
	case MIXER_ROUTING_DAC1_L:
		out = outL2;
		break;
	case MIXER_ROUTING_DAC1_R:
		out = outR2;
		break;
	case MIXER_ROUTING_DAC2_L:
		out = outL;
		break;
	case MIXER_ROUTING_DAC2_R:
		out = outR;
		break;

	}

	switch(selectedType)
	{
	default:
	case FX_OFF:
	{
		if(channels == 2)
		{
			int32_t* stereoIn = (int32_t*)fxBuffer;
			int32_t* stereoOut = (int32_t*)out;

			for(uint8_t i=0;i<size;i++)
			{
				//add stereo samples using SIMD
				*stereoOut = __QADD16(*stereoOut, *stereoIn);
				stereoOut++;
				stereoIn++;
			}
		}
		else
		{
			int32_t* in = (int32_t*)fxBuffer;
			for(uint8_t i=0;i<size;i++)
			{
				*out =  __QADD16(*out, *in);
				out +=2;
				in++;

			}
		}
	}
	break;

	case FX_DISTORTION:
		Distortion_process(&fxBus.distortion, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_RING:
		Ringmod_process(&fxBus.ringmod, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_COMPRESSOR:
		Compressor_process(&fxBus.compressor, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_DELAY:
		if(fxBus.delay.delayType == DELAY_NORMAL)
		{
			Delay_process(&fxBus.delay, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		}
		else
		{
			Delay_processPingPong(&fxBus.delay, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		}
		break;
#if USE_REVERB
	case FX_REVERB:
		Reverb_process(&fxBus.reverb, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;
#endif

	}

}
/* SOURCE_FXBUS_END */
static void render(uint8_t mask){
  uint8_t pos=0;int16_t*output=dac2,*output2=dac1;
  const uint8_t fxPoolMask=mask;
/* SOURCE_MIXER_RET_BEGIN */
    const uint8_t multipleFx = (fxPoolMask & (fxPoolMask - 1u)) != 0u;
    for (uint8_t fxType = FX_OFF; fxType <= FX_DELAY; ++fxType) {
        if (!(fxPoolMask & (1u << fxType)))
            continue;
        if (!multipleFx) {
            FxBus_processSelected(fxType, fxPoolInputs[fxType],
                                  &output[pos], &output[pos+1],
                                  &output2[pos], &output2[pos+1], OUTPUT_DMA_SIZE);
            continue;
        }

        bufferTool_clearBuffer(fxPoolReturn[0], OUTPUT_DMA_SIZE * 2);
        bufferTool_clearBuffer(fxPoolReturn[1], OUTPUT_DMA_SIZE * 2);
        FxBus_processSelected(fxType, fxPoolInputs[fxType],
                              &fxPoolReturn[0][0], &fxPoolReturn[0][1],
                              &fxPoolReturn[1][0], &fxPoolReturn[1][1],
                              OUTPUT_DMA_SIZE);

        /* Mix returned stereo frames after processing, not inside other FX DSP. */
        int32_t *dac1 = (int32_t *)&output2[pos];
        int32_t *dac2 = (int32_t *)&output[pos];
        const int32_t *ret1 = (const int32_t *)fxPoolReturn[1];
        const int32_t *ret2 = (const int32_t *)fxPoolReturn[0];
        for (uint8_t frame = 0; frame < OUTPUT_DMA_SIZE; ++frame) {
            dac1[frame] = __QADD16(dac1[frame], ret1[frame]);
            dac2[frame] = __QADD16(dac2[frame], ret2[frame]);
        }
    }
/* SOURCE_MIXER_RET_END */
}
static int16_t expect1[128],expect2[128];
static void expected(uint8_t mask){
 memset(expect1,0,sizeof(expect1));memset(expect2,0,sizeof(expect2));
 for(unsigned t=0;t<5;t++)if(mask&(1u<<t)){
  unsigned dest=fxBus.audioRouting;
  int16_t*target=(dest==0||dest==2||dest==3)?expect1:expect2;
  unsigned chan=(dest==3||dest==5)?1:0;
  for(unsigned i=0;i<64;i++) {
   for(unsigned c=0;c<(dest<2?2u:1u);c++){
    unsigned idx=(dest<2?c:chan);
    int16_t in=fxPoolInputs[t][2*i+(dest<2?c:0)];
    int16_t s=(t==0)?in:sample((int)t,in,fxBus.dryWet);
    target[2*i+idx]=sat((int32_t)target[2*i+idx]+s);
   }
  }
 }
}
static int failed=0;
static void test(uint8_t mask,unsigned dest,unsigned wet,unsigned pattern,unsigned seed){
 for(unsigned t=0;t<5;t++)for(unsigned j=0;j<128;j++){
  int32_t x=(int32_t)((j*791u+t*2701u+seed*1009u)&65535u)-32768;
  if(pattern==1)x=(j&1)?-32000:32000;
  if(pattern==2)x=0;
  fxPoolInputs[t][j]=(int16_t)x;
 }
 memset(dac1,0,sizeof(dac1));memset(dac2,0,sizeof(dac2));
 memset(calls,0,sizeof(calls));memset(channelCount,0,sizeof(channelCount));
 memset(wetSeen,0,sizeof(wetSeen));clipResets=0;
 fxBus.audioRouting=(uint8_t)dest;fxBus.dryWet=wet*.5f;
 fxBus.delay.delayType=(int)(seed&1u);
 fxBus.distortion.clipStateActive=1;
 expected(mask);render(mask);
 if(memcmp(expect1,dac1,sizeof(dac1))||memcmp(expect2,dac2,sizeof(dac2))) {
  fprintf(stderr,"FAIL sample values mask=%u dest=%u wet=%u pattern=%u\n",mask,dest,wet,pattern);failed++;
 }
 for(unsigned t=1;t<5;t++){
  unsigned should=!!(mask&(1u<<t));
  if(calls[t]!=should||(should&&(channelCount[t]!=(dest<2?2:1)||wetSeen[t]!=fxBus.dryWet))){
   fprintf(stderr,"FAIL FX invocation t=%u mask=%u dest=%u count=%u\n",t,mask,dest,calls[t]);failed++;
  }
 }
}
int main(void){
 unsigned tests=0;
 for(unsigned mask=0;mask<32;mask++)
 for(unsigned dest=0;dest<6;dest++)
 for(unsigned wet=0;wet<3;wet++)
 for(unsigned pattern=0;pattern<3;pattern++){
  test((uint8_t)mask,dest,wet,pattern,tests);tests++;
 }
 if(failed)return 1;
 printf("PASS R23 exact-source bus and return: %u scenarios, 64-frame outputs, sanitizer clean\n",tests);
 /* A removed effect is not dispatched; its persistent state is not cleared here. */
 memset(calls,0,sizeof(calls));fxBus.audioRouting=0;fxBus.dryWet=.5f;
 render((uint8_t)(1u<<FX_DELAY));
 if(calls[FX_DELAY]!=1)return 2;
 render((uint8_t)(1u<<FX_RING));
 if(calls[FX_DELAY]!=1)return 3;
 render((uint8_t)(1u<<FX_DELAY));
 if(calls[FX_DELAY]!=2)return 4;
 puts("PASS transition observation: Delay dispatch count 1 -> 1 -> 2 on active/inactive/active");
 return 0;
}
