#include <stdio.h>
#include "audit.h"
int audit_full=1;unsigned audit_saturations;
int main(void){
 for(unsigned i=0;i<=100000;i++){float x=-32768.99f+(65536.98f*i)/100000.f;assert(isfinite(x));assert(audit_snare(x)==(int16_t)x);}
 float x[]={-1e30f,-65536,-32769,-32768,0,32767,32768,65536,1e30f};
 for(unsigned i=0;i<9;i++)assert(audit_snare(x[i])==(x[i]<-32768?-32768:x[i]>32767?32767:(int16_t)x[i]));
 puts("PASS temporary authorized-clamp arithmetic: 100001 defined-domain values and 9 boundary/over-range controls; both signs");
}
