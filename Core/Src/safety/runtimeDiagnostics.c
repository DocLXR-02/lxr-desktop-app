#include "safety/runtimeDiagnostics.h"
#if LXR_RUNTIME_DIAGNOSTICS
#include "stm32f7xx_hal.h"
/* Flash-resident helpers; no ITCM section, allocation, I/O, resets or locks. */
volatile LxrDiagnostics lxr_diagnostics;
void lxr_diag_init(void)
{
    lxr_diagnostics.version = 1;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55u;
    if(!(DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk)) {
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        lxr_diagnostics.cycle_counter_enabled = !!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
    }
    /* Preserve CYCCNT rather than resetting any other user's counter. */
}
uint32_t lxr_diag_now(void) { return DWT->CYCCNT; }
void lxr_diag_record(unsigned metric, uint32_t begin)
{
    if(!lxr_diagnostics.cycle_counter_enabled) return;
    volatile LxrDiagMetric *m = metric == 0 ? &lxr_diagnostics.render : &lxr_diagnostics.timer_isr;
    lxr_diag_metric(m, begin, lxr_diag_now());
}
void lxr_diag_callback(unsigned stream, unsigned ready)
{
    if(stream >= 2) return;
    volatile LxrDiagDma *d = &lxr_diagnostics.dma[stream];
    uint32_t now = lxr_diag_now();
    if(lxr_diagnostics.cycle_counter_enabled && d->callbacks && now - d->last_cycle > 2u * LXR_DIAG_BUDGET)
        lxr_diag_increment(&d->long_intervals);
    d->last_cycle = now;
    lxr_diag_increment(&d->callbacks);
    if(!ready) lxr_diag_increment(&d->not_ready);
}
void lxr_diag_rearm(unsigned stream, unsigned status)
{
    if(stream < 2 && status != 0) lxr_diag_increment(&lxr_diagnostics.dma[stream].rearm_errors);
}
#endif
