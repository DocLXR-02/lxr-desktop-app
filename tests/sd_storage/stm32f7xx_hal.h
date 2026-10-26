#ifndef SD_HOST_HAL_H
#define SD_HOST_HAL_H
#include <stdint.h>
typedef struct { uint32_t CTRL; } SysTick_Type;
extern SysTick_Type host_systick;
#define SysTick (&host_systick)
#define SysTick_CTRL_ENABLE_Msk 1u
#define SysTick_CTRL_TICKINT_Msk 2u
extern uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
static inline uint32_t __get_IPSR(void){return host_ipsr;}
static inline uint32_t __get_PRIMASK(void){return host_primask;}
static inline uint32_t __get_BASEPRI(void){return host_basepri;}
static inline uint32_t __get_FAULTMASK(void){return host_faultmask;}
uint32_t HAL_GetTick(void);
typedef struct { uint32_t LogBlockNbr,LogBlockSize; } HAL_SD_CardInfoTypeDef;
#endif
