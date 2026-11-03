/* R18 output-crossover regression: production helper vs independent long-double model. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include DISTORTION_SOURCE

static long double state[2];
static int16_t model(unsigned channel, int16_t dry, int16_t wet, float m, float v) {
    const long double d=(long double)dry / 32768.L;
    const long double w=(long double)wet / 32768.L;
    const long double q=w-(long double)v*d;
    state[channel]+=(long double)0.028067915544701783*(q-state[channel]);
    const long double raw=(1.L-(long double)m)*d+(long double)m*(w-state[channel]);
    const long double threshold=1.L-0.08L*(long double)m;
    const long double ab=fabsl(raw);
    long double z=raw;
    if(m>0.f && ab>threshold) {
        const long double ex=ab-threshold,sp=1.L-threshold;
        const long double mag=threshold+sp*ex/(ex+sp);
        z=raw<0.L?-mag:mag;
    }
    int32_t scaled=(int32_t)(z*32768.L);
    if(scaled>32767)scaled=32767;
    if(scaled< -32768)scaled=-32768;
    return (int16_t)scaled;
}
static uint32_t rng=0xaacbaa93u;
static uint32_t step_rng(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(void) {
    MasterDistortion d; memset(&d,0,sizeof(d));Distortion_init(&d);
    assert(sizeof(d.clipV2DifferenceLow)==16);
    for(unsigned i=0;i<75000;i++) {
        unsigned c=i&1;
        if(i%15000==0){Distortion_beginClip(&d);state[0]=state[1]=0.L;}
        int16_t dry=(int16_t)step_rng(), wet=(int16_t)step_rng();
        float v=(float)(step_rng()%128u)/127.f;
        float amount;
        switch(i%8) {
            case 0:amount=0.f;break;
            case 1:amount=1.f;break;
            case 2:amount=.25f;break;
            case 3:amount=.50f;break;
            case 4:amount=.75f;break;
            default:amount=(float)(step_rng()%128u)/127.f;break;
        }
        d.p3=v;
        const int16_t actual=Distortion_clipV2Mix(&d,c,dry,wet,amount);
        const int16_t expected=model(c,dry,wet,amount,v);
        int delta=(int)actual-(int)expected;
        if(delta<0)delta=-delta;
        assert(delta<=1);
        assert(isfinite(d.clipV2DifferenceLow[c]));
        if(amount==0.f)assert(actual==dry);
        if(amount>0.f)assert(actual>-32768 && actual<32767);
    }
    /* Independent CV semantics: with CLP VOL=0, true wet=0 and cleared
       memory, fully wet input must be exactly silent for arbitrary dry. */
    for(unsigned i=0;i<10000;i++) {
        unsigned c=i&1;
        Distortion_beginClip(&d);
        d.p3=0.f;
        assert(Distortion_clipV2Mix(&d,c,(int16_t)step_rng(),0,1.f)==0);
    }
    puts("PASS R18 output crossover: 75,000 independent long-double cases (<=1 LSB), dry identity, soft ceiling, reset and VOL=0");
    return 0;
}
