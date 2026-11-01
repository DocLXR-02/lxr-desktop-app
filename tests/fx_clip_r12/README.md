# R12 independent mono CLP offline checks

Base: 64263ed25f2e9c7f5d880b493d943bfba29271a8. Run from repository root:

    python3 tests/fx_clip_r12/scope.py
    python3 tests/fx_clip_r12/component.py
    python3 tests/fx_clip_r12/run.py
    python3 tests/fx_clip_r12/regressions.py
    python3 tests/fx_clip_r12/renders.py

Native GCC, Python3.12+ and NumPy2.3.5. ASan/UBSan/bounds/float-cast-overflow
are nonrecovering. LeakSanitizer is not exercised (detect_leaks=0).
Production source/mixer/voice paths execute with deterministic RNG/ADC/control
shims. The inherited CLZ(0) host boundary uses its explicitly documented ARM
zero result32, not a production fix. User-sample synthesis is unsupported and
aborts. Full firmware boot, MIDI modulation scheduler, physical DMA/SD/morph
recall and device CPU/stack/thermal performance are not claimed.

Exact independent mono state/return oracle: isolated production mono CLP state
per voice, compared every block with the actual six-path integration; post MIX
and PAN/saturating output arithmetic are checked separately. R11 is the pinned
behavioral/acoustic comparator, not a stereo-state equality oracle. Both real
velocity branches, selected positive MIX levels, exact-zero/frozen-decimator,
returning notes, actual smoother state, route masks and reset endpoints execute.
Moderate filters are selected separately; known unstable filters and original
non-CLP snare overflow remain unresolved. Other FX comparisons use real effect
implementations, restricting Delay to stereo and Compressor to its initialization
default. They do not repair the independently known inherited failures.

Historical R11/R10 assertions remain unchanged. Their old sole-route or shared
Maximum integration policy is deliberately replaced only for CLP; numeric DSP
assertions are reused without weakening them. Each command retains logs and
machine-readable results under build/r12-*. CI evidence contains source, ELF/map,
commands, results and paired synthetic WAVs; it expires after90days unless archived.
No test is a device-installation approval.
