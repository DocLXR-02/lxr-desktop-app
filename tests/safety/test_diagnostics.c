#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f7xx_hal.h"
#include "safety/runtimeDiagnostics.h"
TestDwt test_dwt;
TestDebug test_debug;
#if LXR_RUNTIME_DIAGNOSTICS
int main(void)
{
    test_dwt.CTRL=DWT_CTRL_NOCYCCNT_Msk;
    lxr_diag_init(); assert(!lxr_diagnostics.cycle_counter_enabled);
    lxr_diag_record(0,0); assert(!lxr_diagnostics.render.count);
    test_dwt.CTRL=0; test_dwt.CYCCNT=123;
    lxr_diag_init(); assert(lxr_diagnostics.cycle_counter_enabled && test_dwt.CYCCNT==123);
    lxr_diag_metric(&lxr_diagnostics.render,UINT32_MAX-4,5);
    assert(lxr_diagnostics.render.count==1 && lxr_diagnostics.render.max_cycles==10);
    lxr_diag_metric(&lxr_diagnostics.render,0,LXR_DIAG_BUDGET);
    assert(!lxr_diagnostics.render.over_budget);
    test_dwt.CYCCNT=LXR_DIAG_BUDGET+1; lxr_diag_record(0,0);
    assert(lxr_diagnostics.render.over_budget==1);
    test_dwt.CYCCNT=40; lxr_diag_record(1,10);
    assert(lxr_diagnostics.timer_isr.max_cycles==30);
    lxr_diagnostics.render.count=UINT32_MAX; lxr_diagnostics.render.over_budget=UINT32_MAX;
    lxr_diag_metric(&lxr_diagnostics.render,0,UINT32_MAX);
    assert(lxr_diagnostics.render.count==UINT32_MAX && lxr_diagnostics.render.over_budget==UINT32_MAX);
    test_dwt.CYCCNT=UINT32_MAX-4; lxr_diag_callback(0,1);
    test_dwt.CYCCNT=5; lxr_diag_callback(0,0);
    assert(lxr_diagnostics.dma[0].callbacks==2 && lxr_diagnostics.dma[0].not_ready==1);
    assert(!lxr_diagnostics.dma[0].long_intervals);
    test_dwt.CYCCNT=2*LXR_DIAG_BUDGET+6; lxr_diag_callback(0,1);
    assert(lxr_diagnostics.dma[0].long_intervals==1);
    lxr_diag_rearm(0,0); lxr_diag_rearm(0,1); lxr_diag_rearm(0,2);
    assert(lxr_diagnostics.dma[0].rearm_errors==2);
    lxr_diagnostics.dma[0].callbacks=UINT32_MAX; lxr_diagnostics.dma[0].not_ready=UINT32_MAX;
    lxr_diagnostics.dma[0].rearm_errors=UINT32_MAX;
    lxr_diag_callback(0,0); lxr_diag_rearm(0,1);
    assert(lxr_diagnostics.dma[0].callbacks==UINT32_MAX && lxr_diagnostics.dma[0].not_ready==UINT32_MAX && lxr_diagnostics.dma[0].rearm_errors==UINT32_MAX);
    lxr_diag_callback(2,0); lxr_diag_rearm(2,1);
    puts("PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state");
}
#else
int main(void)
{
    int value=0;
    LXR_DIAG_INIT(); LXR_DIAG_START(unused);
    LXR_DIAG_END(++value,unused); LXR_DIAG_CALLBACK(++value,++value);
    LXR_DIAG_REARM(++value,++value);
    assert(value==1 && test_debug.DEMCR==0);
    puts("PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once");
}
#endif
