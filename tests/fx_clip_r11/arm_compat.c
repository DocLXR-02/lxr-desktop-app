#define main fixture_main
#include "voice_fixture.c"
#undef main
int main(void){
 for(unsigned w=0;w<5;w++)for(unsigned i=0;i<7;i++){
  const int mods[]={-32768,-100,-1,0,1,100,32767};OscInfo o={0};o.phase=0x12345678;o.tableOffset=4;o.fmMod=1;int16_t y,m=mods[i];
  if(!w)calcFmSineBlock(&o,&m,&y,1,1);else if(w==4)calcPwmFmBlock(&o,&m,&y,1,1,.5);else calcFmBlock(&o,w==1?triTable:w==2?recTable:sawTable,&m,&y,1,1);
  printf("FM %u %d %d\n",w,m,y);
 }
 unsigned phases[]={0,0x7fffff00,0x7fffffff,0x80000000,0xc0000000,0xffff0000,0xffffffff};
 for(unsigned r=4;r<=10;r+=6)for(unsigned k=0;k<7;k++)for(unsigned p=0;p<3;p++){
  OscInfo o={0};o.phase=phases[k];o.tableOffset=r;int16_t y;float pwm=p*.5f;calcPwmBlock(&o,&y,1,1,pwm);uint32_t q=osc_finitePhaseU32(phases[k]+pwm*(UINT32_MAX/2));printf("PWM %u %u %.1f %u %d\n",r,phases[k],pwm,q,y);
 }
}
