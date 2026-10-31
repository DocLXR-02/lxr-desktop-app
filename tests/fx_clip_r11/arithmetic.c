#define main voice_fixture_main
#include "voice_fixture.c"
#undef main
static int16_t periodic(const int16_t *t,uint32_t phase){unsigned k=phase>>22;return (int32_t)t[k]+(((int64_t)(t[(k+1)&1023]-t[k])*(phase&0x3fffff))>>22);}
int main(void){
 const int16_t (*tables[3])[1024]={triTable,recTable,sawTable};unsigned long checks=0;
 const unsigned fractions[]={0,1,0x1fffff,0x200000,0x3fffff};
 for(unsigned t=0;t<3;t++)for(unsigned row=0;row<11;row++)for(unsigned k=0;k<1024;k++)for(unsigned f=0;f<5;f++){
  uint32_t phase=(k<<22)|fractions[f];OscInfo o={0},mod={0};o.phase=phase;o.tableOffset=row;int16_t y;
  calcWavetableOscBlock(&o,tables[t],&y,1,1);assert(y==periodic(tables[t][row],phase));
  assert(calcWavetableOsc(&o,tables[t])==y);int16_t zero=0;calcFmBlock(&o,tables[t],&zero,&y,1,1);assert(y==periodic(tables[t][row],phase));assert(calcFm(&o,&mod,tables[t])==y);checks+=4;
 }
 float probes[]={-32768,-8.91f,-.5f,0,.5f,12.9f,32767,0x1.fffffep31f,0x1p32f,0x1.8p32f};uint32_t expected[]={0,0,0,0,0,12,32767,0xffffff00,UINT32_MAX,UINT32_MAX};
 for(unsigned i=0;i<sizeof probes/sizeof *probes;i++)assert(osc_finitePhaseU32(probes[i])==expected[i]);
 uint32_t phases[]={0,0x12345678,0x7fffffff,0x80000000,0xbfffffff,0xc0000000,UINT32_MAX};
 for(unsigned row=0;row<11;row++)for(unsigned p=0;p<7;p++)for(unsigned w=0;w<3;w++)for(int fm=-32768;fm<=32767;fm+=65535){
  float pwm=w*.5f;OscInfo o={0};o.phase=phases[p];o.tableOffset=row;o.fmMod=1;int16_t y,m=fm;calcPwmFmBlock(&o,&m,&y,1,1,pwm);
  uint32_t phase=phases[p]+((fm>0?(unsigned)fm:0)<<19);float sum=phase+pwm*(UINT32_MAX/2);uint32_t p2=sum>=0x1p32f?UINT32_MAX:(uint32_t)sum;
  assert(y==host_sat((int32_t)periodic(sawTable[row],phase)-periodic(sawTable[row],p2),16));
  o.phase=phases[p];calcPwmBlock(&o,&y,1,1,pwm);sum=phases[p]+pwm*(UINT32_MAX/2);p2=sum>=0x1p32f?UINT32_MAX:(uint32_t)sum;assert(y==host_sat((int32_t)periodic(sawTable[row],phases[p])-periodic(sawTable[row],p2),16));checks+=2;
 }
 for(unsigned wave=2;wave<14;wave++)for(unsigned v=0;v<128;v++){
  TransientGenerator tg={0};tg.waveform=wave;tg.pitch=1;tg.volume=v/127.f;int16_t y[64];transient_calcBlock(&tg,y,64);
  for(unsigned i=0;i<64;i++)assert(y[i]==(int16_t)(tg.volume*((int32_t)transientData[wave-2][i]*256)));checks+=64;
 }
 printf("PASS production arithmetic: %lu interpolation/PWM/transient comparisons; finite conversion boundaries\n",checks);return 0;
}
