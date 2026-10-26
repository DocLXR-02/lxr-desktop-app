#ifndef LXR_TEST_HAL_H
#define LXR_TEST_HAL_H
#include <stdint.h>
typedef struct { uint32_t CTRL, CYCCNT, LAR; } TestDwt;
typedef struct { uint32_t DEMCR; } TestDebug;
extern TestDwt test_dwt;
extern TestDebug test_debug;
#define DWT (&test_dwt)
#define CoreDebug (&test_debug)
#define CoreDebug_DEMCR_TRCENA_Msk (1u << 24)
#define DWT_CTRL_NOCYCCNT_Msk (1u << 25)
#define DWT_CTRL_CYCCNTENA_Msk 1u
#endif
