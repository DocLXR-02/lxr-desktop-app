# R13 R7-derived feedback-fader candidate: build and validation report

Date: 2026-11-02. This is a **non-Codex GitHub Actions build**, not a firmware release, device test, or sound acceptance. This report is maintained on a separate report-only branch; the firmware source branch is independent.

## Source identity and authorized scope

- Repository: `DocLXR-02/lxr-desktop-app`
- Immutable R7 baseline: `6d7cb7f7763158ed9abc465a72df7f2560f26b57` (`fx/clp-parallel-bass-r7`); preserved
- R13 candidate source: `149320f082f79edcb6a029f039fdb2234588ead7` on `fx/clp-r7-feedback-fader-r13`
- [R7-to-R13 comparison](https://github.com/DocLXR-02/lxr-desktop-app/compare/6d7cb7f7763158ed9abc465a72df7f2560f26b57...149320f082f79edcb6a029f039fdb2234588ead7)
- CI: [run 41505923432](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41505923432), attempt 1, **success**
- Toolchain: Arm GNU Toolchain 14.3.Rel1 (`arm-none-eabi-gcc` 14.3.1 20250623), Release `-O3`; matched to R7 build evidence
- No merge, force push, release, hardware flash, or instrument test. `main`, R4, R7 and R11 protected baselines were not changed.

The **only DSP behavior change** is a control of the existing CLP feedback tap for exactly one FX-routed voice, with normalized physical fader travel `u`:
`G_FB = min(1, 2*u)`. This preserves R7 feedback at 50–100% travel and linearly decreases feedback from 50% to 0%. The implementation reads the existing smoothed squared fader state `q` and maps it using `min(1, 2*sqrtf(q))`, with a 64-frame linear interpolation between the existing voice-smoother values before and after voice rendering. This is an approximation for moving controls, not a guarantee of exact continuous-time tracking.

The R5 no-input feedback gate, R7 parallel bass, output gain, distortion parameters, other effects, and dry/wet topology are unchanged. With zero or multiple configured FX routes, the feedback factor remains unity (R7 behavior). Visible build identification changed to `LXR LAB R13` on LCD line 1; line 2 is blank.

Production source edits: `Core/Inc/Soundengine/DSPAudio/FX/Distortion.h`, `Core/Src/Soundengine/DSPAudio/FX/Distortion.c`, `Core/Src/Soundengine/DSPAudio/mixer.c`, `Core/Src/main.c`. Additional branch-only files are `.github/workflows/clp-r13-build.yml` and `tests/fx_clip_r13/package.py`. The latter are build/evidence machinery, not production sound changes.

## CI, tests, and evidence

- Native production-DSP inherited CLP detector/idle checks, R7 state and independent-bass checks, actual FX-bus dispatch checks: **PASS**.
- Existing `tests/bug001`, `tests/kit_load`, `tests/project_storage`, `tests/euclidean`: **PASS**.
- Exactly one Release ARM build and link, memory-region fit: **PASS**.
- Image package script verified the magic/header, internal checksum, reset and stack vectors, all ELF load segments and bounds, R13 splash marker, and ZIP membership: **PASS**.
- The downloaded image, ZIP, and ELF were **independently rechecked** by a separate Python verification against the actual bytes, including four flash-mapped load segments, payload coverage, image checksum, and both SHA-256 hashes: **PASS**.
- Preceding local (not CI) R13 source probes: 101 physical fader positions, 441 interpolation cases, all 64 FX-route masks, five bit-identical full-feedback DSP render comparisons, and independent CLP bass-state checks: **PASS**. Do not attribute these separate probes to this CI run.
- Recorded and retained: [source/build evidence artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41505923432/artifacts/13171395394) (compiler/configuration/build and regression logs, ELF/MAP, source archive, verification JSON, hashes).
- [Image artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41505923432/artifacts/13171380456) (IMG and ZIP).

No physical listening, audio-callback deadline, CPU cycle, DMA/IRQ timing, high-water stack, long-term stability, or measured musical fade behavior has been performed. The feedback oscillator may still cut out discontinuously at a threshold. Full-feedback waveform equality in the local probes is a narrow conditional result, not a claim of audible equality across fader positions.

## Matched static resource ledger

| Region | R7 | R13 | Delta | R13 remaining |
|---|---:|---:|---:|---:|
| ITCM | 15104 B | 15536 B | **+432 B** | 848 B |
| DTCM static | 107416 B | 107424 B | **+8 B** | 23648 B gap to top-of-DTCM stack pointer; actual stack usage unmeasured |
| General SRAM static | 274648 B | 274648 B | 0 B | 118568 B before heap demand |
| Linker FLASH | 235012 B | 235092 B | +80 B | 780716 B |
| Complete application flash payload | 287112 B | 287624 B | **+512 B** | not a direct CPU metric |

Important trade-off: This simpler control avoids the R8 256-byte DTCM per-frame gain buffer, but the extra ITCM instruction code still costs **432 B** relative to R7. Savings versus R8 are therefore limited in ITCM. Real-time CPU impact remains **UNMEASURED**. R7 and R13 differ in intended audible feedback behavior; neither is a replacement for hardware listening comparison.

## Delivery and integrity

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R13.img` | 287640 | `970dc0b585912c71006d2f364d42a86e3148ea4cfc215f61b9ce7fbebfa0523e` |
| `LXR_V2_R13.zip` (contains exactly `LXR_V2_R13.img`) | 205843 | `fbb577f7e66e8c945c34bbcbd4f81058859f4cfbc209ee9faf1330e5e821aec0` |
| CI `SOURCE.zip` | — | `46bbfcd16ca767b3dac52e9589b01489fd4f7dd7deebfa0b5f56e9ce9092538e` |

The build is a **candidate for a controlled, low-monitor-volume R7-versus-R13 listening comparison**, subject to the owner's separate device-test decision. No follow-on DSP changes have been authorized.
