# S2 SD HAL/BSP regression tests

Run `python3 tests/sd_hal/run.py` with the prepared ARM compiler on PATH. Native
ASan/UBSan executes actual extracted production function bodies: LL wait helper,
R1/R2/R3/R6/R7/CMD0 response handlers and STOP command; polling HAL read/write,
voltage negotiation, SD_SendStatus and card-state query; BSP read/write/state wrappers. Extraction is
brace-balanced and selects definitions, not declarations. Target preprocessing
provides actual constant values; small CMSIS/register/handle shapes and function
behavior are explicit mocks in host.c. BLOCKSIZE=512 and HAL state enum values
model this target. Complete production translation units are additionally ARM-built.

The register model covers command completion, CMDACT, no response, hardware
command timeout, CRC, the R3 CRC exception, R1/R6 index mismatch, zero/oversized
software timeout, tick wrap, pre-tick rejection, STOP failure, data FIFO success,
polling timeout/CRC failure, preserved context and non-ready state, no direct
retry, BSP timeout fidelity and busy/error distinction. S1's whole-driver tests
also verify an explicit BSP state error immediately latches and cannot be cleared.
No HAL abort/reset/recovery functions are supplied by the test model; an accidental
new dependency fails native linking. Call counts detect replay in tested paths.

LeakSanitizer is disabled because the cloud executor's tracing cannot support it;
ASan/UBSan remain enabled and errors are fatal. A 20-second process deadline bounds
test execution. The model does not emulate electrical timing, card programming,
DMA, hardware clock failure, exact W1C behavior, bus contention or physical
quiescence. It does not simulate a physical SD card or prove firmware audio safety.

Command deadlines are milliseconds capped at 5000; no CPU-clock multiplication
or spin-count fallback. They require foreground, unmasked, enabled/advancing
SysTick. Calls before timebase setup or from disallowed contexts fail closed.
A halted/malfunctioning tick despite enabled control bits, or indefinite interrupt
starvation, remains outside the software timing assumptions. Aggregate startup
and a whole file operation can contain multiple bounded command calls; the
public transfer timeout is not a single shared deadline for all nested calls.
Voltage acquisition also has a 5000 ms outer retry allowance, in addition to
the existing finite trial count; one nested command pair can overrun it before
the next check. SDMMC_DATATIMEOUT is unchanged.

S2 intentionally adds no physical abort, STOP-on-error, peripheral reset or
recovery/retry. Failure state/context reports an incomplete attempt, not proof the
card/data path has stopped. S1's reboot-only disk latch remains in force. Higher
FatFs/save-call propagation and filesystem ownership remain later-phase work.
