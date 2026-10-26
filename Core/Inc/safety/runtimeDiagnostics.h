#ifndef LXR_RUNTIME_DIAGNOSTICS_H
#define LXR_RUNTIME_DIAGNOSTICS_H
#include <stdint.h>
#ifndef LXR_RUNTIME_DIAGNOSTICS
#define LXR_RUNTIME_DIAGNOSTICS 0
#endif
#if LXR_RUNTIME_DIAGNOSTICS
/* Nominal 64-frame period at 216 MHz / 44140.625 Hz; not a measured deadline. */
#define LXR_DIAG_BUDGET 313181u
typedef struct { uint32_t count, max_cycles, over_budget; } LxrDiagMetric;
typedef struct { uint32_t callbacks, not_ready, rearm_errors, long_intervals, last_cycle; } LxrDiagDma;
typedef struct {
    uint32_t version, cycle_counter_enabled;
    LxrDiagMetric render, timer_isr;
    LxrDiagDma dma[2];
} LxrDiagnostics;
extern volatile LxrDiagnostics lxr_diagnostics;
static inline void lxr_diag_increment(volatile uint32_t *value) { if(*value != UINT32_MAX) ++*value; }
static inline void lxr_diag_metric(volatile LxrDiagMetric *metric, uint32_t begin, uint32_t end)
{
    uint32_t duration = end - begin; /* one CYCCNT wrap is supported */
    lxr_diag_increment(&metric->count);
    if(duration > metric->max_cycles) metric->max_cycles = duration;
    if(duration > LXR_DIAG_BUDGET) lxr_diag_increment(&metric->over_budget);
}
void lxr_diag_init(void);
uint32_t lxr_diag_now(void);
void lxr_diag_record(unsigned metric, uint32_t begin);
void lxr_diag_callback(unsigned stream, unsigned ready);
void lxr_diag_rearm(unsigned stream, unsigned status);
#define LXR_DIAG_INIT() lxr_diag_init()
#define LXR_DIAG_START(name) uint32_t name = lxr_diag_now()
#define LXR_DIAG_END(metric, name) lxr_diag_record(metric, name)
#define LXR_DIAG_CALLBACK(stream, ready) lxr_diag_callback(stream, ready)
#define LXR_DIAG_REARM(stream, call) lxr_diag_rearm(stream, (unsigned)(call))
#else
#define LXR_DIAG_INIT() ((void)0)
#define LXR_DIAG_START(name)
#define LXR_DIAG_END(metric, name) ((void)0)
#define LXR_DIAG_CALLBACK(stream, ready) ((void)0)
#define LXR_DIAG_REARM(stream, call) (call)
#endif
#endif
