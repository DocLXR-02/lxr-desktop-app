#ifndef CLP_HOST_GLOBALS_H
#define CLP_HOST_GLOBALS_H
#include <stdint.h>
#include <stddef.h>
#define INDTCM
#define INDTCMZ
#define IN_ITCM_RAM
#define __fp16 float
static inline int32_t host_sat(int32_t x,unsigned b){int32_t hi=(1<<(b-1))-1,lo=-(1<<(b-1));return x>hi?hi:x<lo?lo:x;}
#define __SSAT(x,b) host_sat((x),(b))
static inline int32_t host_add16(int32_t a,int32_t b){uint32_t lo=(uint16_t)host_sat((int16_t)a+(int16_t)b,16);uint32_t hi=(uint16_t)host_sat((int16_t)((uint32_t)a>>16)+(int16_t)((uint32_t)b>>16),16);return (int32_t)(lo|(hi<<16));}
#define __QADD16(a,b) host_add16((a),(b))
#define __PKHBT(a,b,s) ((int32_t)(((uint32_t)(a)&65535u)|((uint32_t)(b)<<(s))))
#endif

