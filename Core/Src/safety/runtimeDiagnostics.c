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
/* DIAG 01: one foreground-owned result, no extra work in audio writers. */
#define LXR_DIAG_SESSION_LIMIT_MS (24u * 60u * 60u * 1000u)
static LxrDiagResult diag_result;
static uint32_t diag_start_tick;
static LxrDiagSessionState diag_state;

LxrDiagSessionState lxr_diag_session_state(void) { return diag_state; }
const LxrDiagResult *lxr_diag_session_result(void)
{
    return diag_state == LXR_DIAG_FROZEN ? &diag_result : 0;
}

uint8_t lxr_diag_session_start(void)
{
    if(__get_IPSR() != 0u || diag_state == LXR_DIAG_RUNNING) return 0;
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    /* The caller must be outside the render path. Reset only these metrics,
     * including the first-gap baseline. Never reset DWT or DMA state. */
    lxr_diagnostics.render = (LxrDiagMetric){0};
    lxr_diagnostics.timer_isr = (LxrDiagMetric){0};
    lxr_diagnostics.dma[0] = (LxrDiagDma){0};
    lxr_diagnostics.dma[1] = (LxrDiagDma){0};
    diag_start_tick = HAL_GetTick();
    __set_PRIMASK(primask);
    if(diag_result.number != UINT32_MAX) ++diag_result.number;
    diag_state = LXR_DIAG_RUNNING;
    return 1;
}

uint8_t lxr_diag_session_stop(void)
{
    if(__get_IPSR() != 0u || diag_state != LXR_DIAG_RUNNING) return 0;
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    diag_result.data = lxr_diagnostics;
    const uint32_t elapsed = HAL_GetTick() - diag_start_tick;
    __set_PRIMASK(primask);
    diag_result.elapsed_ms = elapsed;
    diag_result.duration_limit = elapsed >= LXR_DIAG_SESSION_LIMIT_MS;
    diag_state = LXR_DIAG_FROZEN;
    return 1;
}

uint8_t lxr_diag_session_poll(void)
{
    /* Foreground may be delayed by an existing blocking operation. */
    if(diag_state == LXR_DIAG_RUNNING &&
       (uint32_t)(HAL_GetTick() - diag_start_tick) >= LXR_DIAG_SESSION_LIMIT_MS)
        return lxr_diag_session_stop();
    return 0;
}

#endif
