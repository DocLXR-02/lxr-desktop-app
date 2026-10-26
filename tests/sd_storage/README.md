# S1 SD disk-driver foundation tests

Run `python3 tests/sd_storage/run.py` from the repository root. The runner compiles
`host.c`, which includes the **whole production `sd_diskio.c`**, using the real
local BSP declarations and test-only disk/CMSIS/HAL definitions. It runs native
ASan/UBSan with a 15-second process deadline. LeakSanitizer is disabled because
this executor's tracing does not support it; the driver allocates no memory.

The fixture mocks BSP initialization, sector transfers, card readiness/presence,
geometry, HAL milliseconds, CPU exception/mask registers and SysTick enable bits.
It tests every supported count 1–128, exact end-of-card ranges, finite/permanent
busy, tick wrap, failed BSP transfers, removal, initialization failure, sync,
invalid requests, forbidden caller contexts and persistent failure rejection.
The test-only `reboot()` resets file-static state; production has no fault reset.
After faults, repeated access/status/init/sync requests must not call hardware.
No reset/remount/retry implementations are linked: accidental dependencies fail
linking. There is no real card, media, FatFs core or audio execution here.

## Production contract

Only serialized foreground calls with unmasked, enabled SysTick are supported.
A BSP error, readiness timeout, failed initialization, or detected removal during
use latches NOINIT until reboot. Ordinary status, init, mount/relink and reinsertion
cannot clear the file-static latch. Startup with no card is retryable until an
actual initialization/transfer failure occurs. Invalid parameters/context do not
latch a media fault. First I/O/timeout failure returns RES_ERROR; known absence
returns RES_NOTRDY and NODISK; subsequent valid access returns RES_NOTRDY. BSP
cannot reliably distinguish timeout, busy, CRC, removal or protection in its
collapsed result; S1 never invents a write-protect classification.

The software policy allows 5000 ms for a polling transfer of at most 128 sectors
(64 KiB), plus 5000 ms of post-transfer readiness polling. This is deliberately
generous relative to ordinary card programming/transfer times, but remains an
engineering policy needing supported-card validation, not a universal SD latency
guarantee. The separate SDMMC hardware timeout register is unchanged. Lower HAL
command/status/stop calls may overrun these budgets and are deferred to S2. A
stalled tick despite enabled SysTick, unbounded interrupt starvation, physical
peripheral quiescence, and actual card programming latency are not simulated.

S1 does not repair ignored FatFs/preset errors, shared-FIL cleanup, transaction
atomicity or automatic recovery. It prevents further driver I/O after an observed
serious failure; it cannot undo the uncertain write or promise filesystem integrity.
There are no changes to user file formats or the runtime diagnostic option.
