# FX-bus DRV / CLP: initial diagnosis and improvement candidates

Date: 2026-10-29 (Europe/Vienna)
Task: Investigate the musical usability of FX-bus Drive / Clip without changing firmware.
Repository: https://github.com/DocLXR-02/lxr-desktop-app
Examined R4 source: `ui/kit-load-confirmation`, commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`.
Report-only branch: `codex-reports/2026-10-29-fx-clp-diagnosis`.
Report branch base: `lab-setup`, commit `5829398cea5d337200393757e39a37d4d8148256`.
Report commit: the commit containing this file; no firmware source changes.

## Objective and evidence boundaries

The owner finds the documented self-oscillation practically unusable during performance, but wants to retain the effect's musical potential. This task identifies concrete improvement candidates. It does not authorize implementation, firmware installation, or new device tests.

Sources read: the attached CLP handoff dated 2026-10-28, KnowledgeBase v1.1.1, supplied owner manual (printed page 26), exact R4 source files, current upstream Distortion.c and its file history, and targeted community searches.

DEVICE (previous observations, not newly repeated): loud persistent whistle at certain parameter combinations, including with Voice 1 fader down and transport stopped. Dry/wet affects audibility. Lowering drv, col, ton or vol below a threshold can stop it. Exact numerical settings and routing are still missing.

MANUAL: Clip deliberately supports feedback tones without input. This explains why stop/fader observations alone do not establish a routing bug.

## 1. Confirmed control mapping

Mapping traced through:
- `Core/Inc/menu/menuPages.h:494-495`
- `Core/Src/Soundengine/MIDI/MidiParser.c:1392-1415`
- `Core/Src/Soundengine/FxBus.c:159-163`
- `Core/Src/Soundengine/DSPAudio/FX/Distortion.c:390-604`

| Visible control | CLP implementation | Practical implication |
| --- | --- | --- |
| drv | gain = 1 + drive * 2304 | Changes amplification within the feedback loop as well as distortion. |
| col | previous lastOut * p1 * 0.125 added to input | This is the Clip feedback control. |
| ton | output low-pass coefficient = 0.0653794899545445 + p2 * 0.9241070173442663 | Tone filtering also lies inside the feedback loop. |
| vol | lastOut = tone-filter state * p3 | Sets audible wet level AND the signal returned to the loop. |
| d/w | Applied after lastOut, when mixing wet/dry | Changes the audible mix without directly reducing internal feedback strength. |

The normal integer control path converts values to floats using value / 127 (`MidiParser.c:164`). The mapping describes nominal controls, not every possible modulation or invalid-input path.

Decisive excerpts, stereo path:
```c
signal1 += dist->lastOut[0] * dist->p1 * 0.125; // line 420
const double gain = 1 + dist->drive*2304;      // line 402
dist->lastOut[0] = dist->lp3_state[0] * dist->p3; // line 535
```
The mono path repeats the same coupling at lines 558 and 595.

CODE conclusion: vol is not an independent post-feedback output trim. Its effect on the whistle threshold is structurally explained. This is not proof that every observed onset was caused solely by this coupling.

## 2. Confirmed filter/comment discrepancy

At `Distortion.c:429-445`, the input high-pass comment specifies 7.2 Hz and states the exponential coefficient formula. The implementation uses:

```c
dist->hp1_state[0] += 0.64116641920131193922
    * (signal1 - dist->hp1_state[0]);
signal1 = signal1 - dist->hp1_state[0];
```

`Core/Inc/Soundengine/config.h` specifies REAL_FS = 44140 Hz.

Calculation with Python standard-library math:
- Implemented low-pass state coefficient a = 0.6411664192013119, pole r = 1-a = 0.3588335807986881.
- In the code's stated formula, -ln(r) * 44140 / (2*pi) = 7200 Hz.
- For the commented 7.2 Hz, a would be 0.0010243715341082815.

The subtraction implements H(z) = r * (1-z^-1) / (1-r*z^-1). Calculated isolated-stage gains, relative to its input:
- 60 Hz: -46.4119 dB
- 100 Hz: -41.9754 dB
- 1000 Hz: -22.0580 dB
- 7200 Hz: -7.8582 dB

These are calculations for this stage only, not measurements of the complete effect or physical device. 7200 Hz is the exponential pole parameter, not a claim of an exact digital -3 dB cutoff.

CODE: the coefficient and comment disagree by a factor of 1000 in the stated frequency formula.
HYPOTHESIS: the implementation may contain an Hz/kHz calculation error. Alternatively, the implementation may be deliberate and the comment wrong. The code alone does not establish intended sound.
INFERENCE: strong bass attenuation in this stage makes filter/gain staging especially relevant to kick processing. Changing it could substantially alter the effect and destabilize existing parameter combinations; do not change it blindly.

## 3. Confirmed diode-table boundary defect

Community lead: https://github.com/erica-synths/lxr-desktop-app/issues/3 (finding D5).

Independently inspected in R4 `Distortion.c:18,129-143`:
- table_1n914 has 257 entries, valid indices 0 through 256.
- diode() clamps input to [-1,1].
- At +1, index = 256, but interpolation reads table_1n914[itg+1], i.e. index 257.
- The interpolation fraction is zero, but that does not make the out-of-bounds access valid C.
- The CLP slew state is bounded at +/-4.2 and passed into diode(), so the +1 clamp endpoint is relevant to normal clipping.

HOST TEST: copied the unchanged table and clamp/fnms/lerp/diode functions into a temporary C probe with main calling diode(1.0f).
Command:
```sh
cc -O0 -g -fsanitize=undefined,bounds -fno-sanitize-recover=all /tmp/lxr_clp_diode_probe.c -lm -o /tmp/lxr_clp_diode_probe
/tmp/lxr_clp_diode_probe
```
Compilation: exit 0.
Probe: exit 1, sanitizer message:
```text
/tmp/lxr_clp_diode_probe.c:92:43: runtime error: index 257 out of bounds for type 'float [257]'
```

This confirms an invalid access in the isolated original function. It does not establish a NaN, memory value, whistle cause, or audio failure on the ARM device. Fixing it is a distinct correctness task; it should not be marketed as eliminating intentional self-oscillation.

## 4. Upstream/community check

- Current upstream Distortion.c returned identical text to examined R4 Distortion.c. This comparison concerns fetched text, not a verified binary or byte-identical blob claim.
- Upstream file history returned only initial commit `e6cfbe904ec88d3f9f7571a3dfeb3d57d81957f7`, dated 2026-10-20.
- Official issue #3 documents the diode issue.
- Repository search returned 24 lxr-desktop-app repositories including upstream and owner forks. Targeted commit searches across that returned set for clip/clp/distortion/feedback/diode returned no matches; targeted PR search returned a related RNBO port, not a ready-to-use LXR hardware usability fix.
- https://github.com/peterswimm/lxr-desktop-app/pull/1 describes an RNBO port and host comparison of chaotic RAT feedback by loudness. It is not a validated fix for this device.
- No directly applicable ready-made CLP usability fix was identified in these searches. Coverage is limited by search indexing, query matching, and branch visibility; this is not proof no fix exists.
- The GitHub fetch route rejected the forks collection endpoint. Repository search was used instead. Initial public web search was noisy; a second search engine verified primary-source availability.

## 5. Improvement candidates, not implementation decisions

1. Independent output level after feedback. Let col control feedback and vol control audible wet level. This directly addresses the confirmed coupling, but requires explicit loop-gain calibration; simply removing p3 from the return path would effectively select the old maximum-volume loop behavior.
2. A more usable feedback control range. Measure onset versus drv/ton and redesign col's curve only after identifying the desired musical range. No universal safe threshold has been established.
3. Resolve the high-pass discrepancy through reference analysis and offline audio comparisons. Compare bass body, transient response, spectrum, and feedback onset. Do not assume that substituting the commented value will automatically improve the sound.
4. Correct the diode endpoint independently, with regression coverage for both endpoints and adjacent values, before interpreting high-drive DSP results as fully defined behavior.

Preserving the documented feedback character is a viable goal. A limiter alone would not remove a sustained tone or decouple loudness from feedback. A gate could suppress intentionally quiet material or tails; it is not yet justified by the current evidence.

Compatibility tradeoff: changed parameter semantics can alter existing kits. Any later implementation must explicitly choose how legacy sounds are preserved or migrated. No parameter storage/UI expansion has been selected.

## 6. Changes, validation and limitations

Changes: this English report only, on a report-only branch based on lab-setup; no firmware source edits, no build image, no flash, no new hardware test, no main/R4 ref changes. A ZIP copy is provided for handoff.

Executed:
- Local text reads of handoff and KnowledgeBase.
- pdftotext -layout extraction of supplied manual.
- GitHub read/search operations described above.
- Python analytical coefficient/frequency-response calculation (exit 0).
- Isolated diode sanitizer probe (expected diagnostic failure described above).

Not executed: complete DSP host render, firmware build, audio comparison, resource measurement, fresh hardware reproduction, official-v1.71 A/B. The examined public source cannot establish the DSP implementation inside the supplied official v1.71 binary.

Open questions:
- Exact displayed drv/col/ton/vol/d/w/out values and voice routing in a problematic saved state.
- Is the main target a controllable drum distortion with optional feedback, or a deliberately playable self-oscillating feedback instrument?
- Which tonal qualities should be retained?
- Intended input-filter design and compatibility policy for existing kits.

Next action: obtain the musical target and an existing problematic parameter snapshot; then design offline comparisons with independent output control and investigated filter alternatives. Device tests and firmware changes require separate explicit authorization under the repository instructions.
