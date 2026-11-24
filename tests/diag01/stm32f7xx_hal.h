#ifndef DIAG_HOST_HAL_H
#define DIAG_HOST_HAL_H
#include <stdint.h>
typedef struct { uint32_t DEMCR; } HostDebug;
typedef struct { uint32_t LAR, CTRL, CYCCNT; } HostDwt;
extern HostDebug host_debug;
extern HostDwt host_dwt;
extern uint32_t host_tick, host_mask, host_ipsr, host_locks, host_unlocks;
#define CoreDebug (&host_debug)
#define DWT (&host_dwt)
#define CoreDebug_DEMCR_TRCENA_Msk (1u<<24)
#define DWT_CTRL_NOCYCCNT_Msk (1u<<25)
#define DWT_CTRL_CYCCNTENA_Msk 1u
static inline uint32_t HAL_GetTick(void) { return host_tick; }
static inline uint32_t __get_IPSR(void) { return host_ipsr; }
static inline uint32_t __get_PRIMASK(void) { return host_mask; }
static inline void __disable_irq(void) { host_mask=1; ++host_locks; }
static inline void __set_PRIMASK(uint32_t x) { host_mask=x; ++host_unlocks; }
#endif
