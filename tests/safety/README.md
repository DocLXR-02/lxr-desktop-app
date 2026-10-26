# Phase B bounded heap and optional diagnostics

Run `python3 tests/safety/run.py` with native GCC/ASan/UBSan. Tests compile the
actual `_sbrk` source with host linker-symbol aliases, and the actual diagnostic
implementation with fake DWT registers. Tests never operate hardware. Existing
Bug 001 and Euclidean suites remain required.

The strong `_sbrk(int)` replaces the unbounded libnosys implementation. It uses
existing `_end` and `_estack` symbols; startup moves the active MSP to DTCM before
C initialization. SRAM from the 8-byte-aligned `_end` to exclusive `_estack` is
available to the heap. No linker/startup change is made. The break remains byte
granular, as required by sbrk; malloc owns chunk alignment. Invalid growth,
shrink below start, corrupted bounds and integer wrap are rejected with
`(caddr_t)-1` and `errno=ENOMEM`, leaving the break unchanged. This does not make
newlib reentrant or make formatting on allocation failure safe. No allocator or
formatting call is added to interrupt handlers. Existing USB configuration can allocate/free in USB interrupt context while main
formatting may allocate. Newlib locks remain no-ops: concurrent allocator access
is an unresolved deployment blocker. This patch bounds the program break only;
it does not make allocator metadata or `_sbrk` calls reentrant. Do not enable
concurrent allocation workloads without a separately reviewed solution.

Diagnostics default OFF in CMake (`LXR_RUNTIME_DIAGNOSTICS`) and in the header.
Normal presets remain unchanged. Example isolated build:

```sh
cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin \
  -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-release --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release
```

For Debug use a separate directory and `-DCMAKE_BUILD_TYPE=Debug`. Do not flash or
upload the resulting binaries. Default-off hooks evaluate no diagnostic arguments;
the original DMA rearm call still executes exactly once. New code uses fixed BSS
and flash-resident helpers, with no heap, logging, locks or additional interrupts.

When enabled, `lxr_diagnostics` can be inspected later via the matching ELF and an
explicitly authorized debugger. There is no UI, serial command or file output.
Fields are volatile 32-bit observations; a running multi-field debugger read is
not an atomic snapshot. Do not zero counters while audio is running. Metrics have
separate owners under existing scheduling: render is serviced with TIM2 masked in
main context or by TIM2; timer metric is TIM2-only; DMA metrics belong to each
same-priority callback. This assumes those original scheduling rules remain true.

- `version`: diagnostic schema 1, not a firmware build ID.
- `cycle_counter_enabled`: DWT supports/enables CYCCNT; not proof the clock runs
  under every debug/security/power state. Initialization unlocks DWT and enables
  trace/counting without resetting CYCCNT. Disabled builds touch no DWT register.
- `render`: successful render count, maximum elapsed cycles, over-budget count.
  Measured around the actual mixer work through readiness publication.
- `timer_isr`: analogous metric around sequence/render/UART work, excluding GPIO
  edges, IRQ entry/exit and the final statistics update.
- `dma[0]` = SPI2, `dma[1]` = SPI3: callback count, not-ready observation count,
  HAL rearm non-OK return count, intervals longer than two nominal block periods,
  and last timestamp. The existing global error counter remains untouched.

Counts saturate at UINT32_MAX. Durations use modulo-32-bit subtraction; intervals
must be shorter than one full CYCCNT period (~19.9 seconds at 216 MHz). The nominal
budget 313181 cycles assumes 216 MHz and 64 frames at 44140.625 Hz. It is a reporting
threshold, not a measured hardware deadline. Metrics include preemption within the
measurement span. No cycle sum/average is maintained, avoiding a 64-bit hot counter.

The counters do not identify sample freshness, repeated generation identities,
request-to-ready latency, every DMA hardware error, or confirmed analog underruns.
A missing render caused by a hang does not produce an end timestamp. Readiness
observations can reflect callback order rather than an audible fault. Distinguish
rearm status from transfer errors. Generation sequence tracking, persistent crash
capture and a safe stack watermark remain design-only.

Before authorized hardware evaluation, calibrate empty hooks and DWT behavior,
compare instrumented versus normal workloads, verify actual clock/deadline values,
and correlate both streams with existing GPIO timing. Each hook is bounded O(1),
but flash calls/cache misses and callback latency add unknown runtime cost; no
performance measurements have been made. Require measured margin and investigate
all unexplained events before deployment. Stack/fault diagnostics must be reviewed
separately; never paint arbitrary DTCM or enable a watchdog as part of this test.
