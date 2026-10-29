#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "Soundengine/DSPAudio/FX/Distortion.h"
#define FS 44140
int main(int argc,char **argv){
 if(argc!=3)return 2;unsigned block=atoi(argv[2]);if(!block || block>64)return 2;
 FILE *f=fopen(argv[1],"wb");if(!f)return 3;
 const int rows[][4]={{127,0,0,127},{127,0,127,127},{127,6,0,127},{127,3,127,127},{63,7,127,127},{63,21,0,63},{63,11,127,63},{63,12,63,63},{32,81,0,32},{32,44,63,32},{127,127,127,127},{127,127,0,127}};
 for(unsigned row=0;row<12;row++)for(unsigned scenario=0;scenario<3;scenario++)for(unsigned channels=1;channels<=2;channels++){
  MasterDistortion d;memset(&d,0,sizeof d);Distortion_init(&d);d.distortionType=2;
  d.drive=rows[row][0]/127.f;d.p1=rows[row][1]/127.f;d.p2=rows[row][2]/127.f;d.p3=rows[row][3]/127.f;
  for(unsigned n=0;n<2*FS;){
   unsigned size=2*FS-n<block?2*FS-n:block;
   _Alignas(4) int16_t in[128]={0},out[128]={0};
   for(unsigned i=0;i<size;i++){
    double t=(n+i)/(double)FS,v=0;
    if(scenario==0 || (scenario==1 && t<.5))v=12000*sin(6.283185307179586*63*t);
    if(scenario==2 && t<.25)v=20000*exp(-t*25)*sin(6.283185307179586*(60*t+1.2*(1-exp(-t*40))));
    in[2*i]=in[2*i+1]=(int16_t)v;
   }
   Distortion_process(&d,in,out,channels==2?out+1:NULL,size,channels,.4f);
   if(fwrite(out,sizeof(int16_t),2*size,f)!=2*size)return 4;n+=size;
  }
 }
 return fclose(f)!=0;
}

