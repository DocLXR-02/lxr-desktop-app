/* Source/decimator transitions only; no CLP or full mixer integration claim. */
#define main matrix_fixture_main
#include "audit.c"
#undef main
float mixer_decimation_rate[7],mixer_decimation_cnt[6];int16_t mixer_voice_samples[6];
#include "decimator.inc"
int main(int argc,char **argv){
 assert(argc==7);unsigned v=atoi(argv[1]),m=atoi(argv[2]),vm=atoi(argv[3]),profile=atoi(argv[5]);audit_full=atoi(argv[4]);
 setup(v,0,m/127.f,vm);
 float *vol=v<3?&voiceArray[v].vol:v==3?&snareVoice.vol:v==4?&cymbalVoice.vol:&hatVoice.vol;
 SlopeEg2 *eg=v<3?&voiceArray[v].oscVolEg:v==3?&snareVoice.oscVolEg:v==4?&cymbalVoice.oscVolEg:&hatVoice.oscVolEg;slopeEg2_setDecay(eg,30,0);if(v==5)hatVoice.decayClosed=hatVoice.decayOpen=slopeEg2_calcDecay(30);
 OnePoleFilter *physical=v<3?&voiceArray[v].FaderVolumeFilter:v==3?&snareVoice.FaderVolumeFilter:v==4?&cymbalVoice.FaderVolumeFilter:&hatVoice.FaderVolumeFilter;
 float *target=vol;uint64_t hash=1469598103934665603ull,sourcehash=hash;unsigned held=0,zero_raw=0,postzero=0;int peak=0,step=0,previous=0,last=0;double energy=0;FILE *f=NULL;if(strcmp(argv[6],"-")){f=fopen(argv[6],"wb");assert(f);}
 _Alignas(4) int16_t data[64];
 for(unsigned b=0;b<2048;b++){
  *target=profile==0 && ((b>=8&&b<32)||b>=768)?0:m/127.f;
  float u=profile==0?1:b<128?1:b<512?(512-b)/384.f:b<640?0:b<768?(b-640)/128.f:1;qtarget=u*u;
  if(b==512||b==1024){if(v<3)Drum_trigger(v,127,60);else if(v==3)Snare_trigger(127,60);else if(v==4)Cymbal_trigger(127,60);else HiHat_trigger(127,0,60);}
  if(v<3)calcDrumVoiceAsync(v);else if(v==3)Snare_calcAsync();else if(v==4)Cymbal_calcAsync();else HiHat_calcAsync();
  if(v<3)calcDrumVoiceSyncBlock(v,data,64);else if(v==3)Snare_calcSyncBlock(data,64);else if(v==4)Cymbal_calcSyncBlock(data,64);else HiHat_calcSyncBlock(data,64);
  assert(isfinite(physical->b));
  for(unsigned i=0;i<64;i++){sourcehash=(sourcehash^(uint16_t)data[i])*1099511628211ull;if(*target==0&&data[i])zero_raw++;}
  mixer_decimation_rate[v]=profile==0&&b>=8&&b<32?0:.125f;mixer_decimation_rate[6]=1;mixer_decimateBlock(v,data);
  for(unsigned i=0;i<64;i++){
   if(*target==0&&data[i])held++;
   if(audit_full&&*target==0)data[i]=0; /* proposed downstream endpoint, not production change */
   if(audit_full&&*target==0&&data[i])postzero++;
   int x=data[i];if(x)last=b*64+i+1;if(abs(x)>peak)peak=abs(x);if(abs(x-previous)>step)step=abs(x-previous);previous=x;energy+=(double)x*x;hash=(hash^(uint16_t)x)*1099511628211ull;
  }
  if(f)assert(fwrite(data,sizeof data,1,f)==1);
 }
 if(f)assert(!fclose(f));printf("{\"source_hash\":\"%016llx\",\"decimated_hash\":\"%016llx\",\"peak\":%d,\"rms\":%.12g,\"max_step\":%d,\"last_nonzero\":%d,\"zero_target_raw_nonzero\":%u,\"zero_target_decimator_nonzero\":%u,\"zero_target_after_proposed_gate_nonzero\":%u}\n",(unsigned long long)sourcehash,(unsigned long long)hash,peak,sqrt(energy/(2048*64)),step,last,zero_raw,held,postzero);
}
