# FX CLP hardware-threshold comparison: initial host reference

Date: 2026-10-29 (Europe/Vienna)
Repository: https://github.com/DocLXR-02/lxr-desktop-app
Examined firmware source: R4 commit c870f7249d7ee0d65423bc8b7bfdb422dcc74482, branch ui/kit-load-confirmation.
Report-only branch: codex-reports/2026-10-29-fx-clp-host-thresholds
Report branch base: lab-setup commit 5829398cea5d337200393757e39a37d4d8148256.
Head: the commit containing this report.

## Scope and owner requirements

Investigate the documented Clip feedback behavior without changing the firmware. Preserve the good distortion sound while input is playing, and prevent the extremely loud sustained tone when input disappears. No gate or feedback-suppression implementation was selected or tested.

DEVICE: the owner fixed d/w at 40 and reported the threshold combinations below, in drv / col / ton / vol order. With the Voice 1 fader at zero, the sustained tone appears at the listed setting and ends one control step below the boundary. Sequencer running/stopped made no difference in this test. The duplicate 127 / 11 / 0 / 127 entry was explicitly corrected to 127 / 6 / 0 / 127.

The task interprets the listed col and col-1 as boundary comparison points. The owner's broad phrase about lowering one value is not treated as proof that every individual parameter was separately reduced by one in every case.

## Source and host adaptation

Fetched exact R4 Distortion.c and Distortion.h through the GitHub connector. Built the complete Distortion_process function with host definitions for section attributes, __SSAT, __QADD16 and __PKHBT. No firmware DSP coefficients, gain mappings, or feedback equations were changed.

Important exception: the diagnostic host source bounds the known invalid positive diode-table endpoint:
```c
if (in >= 1.f) {
    endpoint_hits++;
    return table_1n914[256];
}
```
This guard is host-only. It is necessary to avoid the already-confirmed table_1n914[257] undefined access, and means this is a bounded diagnostic reference, NOT a bit-identical run of the untouched firmware. The original retrieved source is included separately in the analysis bundle. Do not interpret matching thresholds as proof that the original invalid read is harmless.

Distortion.c paths:
- diode lookup: approximately lines 129-143
- CLP gain and stereo loop: lines 390-548
- mono loop: lines 550-604

The mappings use drive, p1, p2 and p3 = displayed integer / 127.f; dryWet = 0.4f.
The menu CC send paths and MidiParser.c:164 were checked as initial confirmation of nominal integer-to-float conversion; this is not a complete audit of modulation and preset recall.

## Inputs, measurements and commands

Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0.
Build flags:
```sh
cc -std=c11 -O2 -fno-strict-aliasing -fsanitize=undefined,bounds -fno-sanitize-recover=all tmp/clp-host/probe.c -lm -o /tmp/lxr_clp_host
/tmp/lxr_clp_host > tmp/clp-host/threshold_results.csv
```
Equivalent builds/runs were made for probe_long.c and probe_scan.c. All builds and runs exited 0, without sanitizer diagnostics.

This is a DSP-function test, not the complete LXR engine. It uses 64-sample blocks and:
1. Exact zero input from fresh initialized state.
2. A synthetic decaying pitch-swept sine kick for approximately 0.25 s, then exact zero input. This is NOT a reconstruction or recording of the owner's actual voice.
3. Both mono and stereo; stereo receives identical input on both channels.
4. Independent fresh-state runs at col-1 and col.
5. A warm-state reduction from col to col-1, with zero input.
6. Approximately 3 s and 10 s per stage; RMS taken over the final approximately 1 s.
7. A local col scan from reported col-3 through col+3, with synthetic kick input, mono, approximately 3 s each.

Sample clock convention: 44140 Hz, matching the firmware's REAL_FS convention. Segment lengths are 3*44160 and 10*44160 samples, so durations are slightly longer than exactly 3 and 10 seconds.

Measured output is the resulting int16 wet/dry mix at d/w=40, with empty destination buffers. It is not a physical DAC or master-output measurement. A sustained tone for scan classification means final RMS > 0.001 full-scale (about -60 dBFS). This is an analysis criterion, not a proposed device noise-gate threshold. Observed active RMS values are far above this threshold.

## Results

| drv | ton | vol | Device col onset | Host col onset, kick-seeded local scan | Outcome |
| --- | --- | --- | --- | --- | --- |
| 127 | 0 | 127 | 6 | 6 | Exact |
| 127 | 127 | 127 | 3 | 3 | Exact |
| 63 | 127 | 127 | 6 | 6 | Exact |
| 63 | 0 | 63 | 21 | 21 | Exact |
| 63 | 127 | 63 | 12 | 11 | Host one step earlier |
| 63 | 63 | 63 | 11 | 12 | Host one step later |
| 32 | 0 | 32 | 81 | 81 | Exact |
| 32 | 63 | 32 | 43 | 44 | Host one step later |

Five of eight reported boundaries match exactly after synthetic-kick seeding. At those five boundaries, the measured final quantized output is zero both at col-1 and after lowering the active oscillator to col-1. The other three do not yet meet the desired calibration criterion.

Examples from nominal 10 s mono kick-seeded stages:
- 127 / 6 / 0 / 127: final output RMS approximately 0.10543, col-1 RMS 0.
- 127 / 3 / 127 / 127: RMS approximately 0.14562, col-1 RMS 0.
- 63 / 12 / 127 / 63: col=12 RMS approximately 0.07069; col=11 remains active at approximately 0.05392.
- 63 / 11 / 63 / 63 and 32 / 43 / 63 / 32: output remains zero at the owner's onset setting; local scans find the next col step active.

Mono and identically driven stereo show the same pass/fail outcomes in these tests. Increasing the observation period from approximately 3 to 10 s did not resolve the three mismatches.

State dependence matters: for ton=0 at 63 / 21 / 0 / 63 and 32 / 81 / 0 / 32, the synthetic kick triggers persistent output, whereas the exact-zero fresh-state run stays quantized silent within the tested interval. Thus a silent fresh-state test alone can miss a relevant sustained-feedback state.

The endpoint guard was exercised during the test sequences (see endpoint_hits in CSV). Its counter includes startup kick processing as well as silent stages; it must not be read as a count exclusively during self-oscillation.

## Conclusions and limits

HOST: the reference reproduces the general input-loss oscillation, exact thresholds for five supplied cases, and immediate-neighbor extinction in those matching cases.
HOST: three one-step threshold disagreements remain.
INFERENCE: direct input/feedback interaction is relevant, and initial state/input history can affect onset.
OPEN: precise cause of the discrepancies. Potential checks include actual saved parameters, modulation, routing/channel conditions, voice waveform/history, compilation semantics, and the effect of defining the formerly invalid diode endpoint. These are possibilities, not findings.
OPEN: no actual project folder or recorded audio was supplied in this turn; no claim of reproducing the owner's original kick sound is made.
Not tested: arbitrary stereo inputs, complete mixer/fader/transport implementation, exact physical output level, ARM build or CPU/memory cost, firmware installation, alternative DSP designs.

No conclusion is made that the input high-pass coefficient should be changed. No limiter/gate has been added, and preserving the active-input sound has not yet been demonstrated for any proposed modification.

## Changes and publication

No firmware source or R4/main branch changes. Local analysis harnesses and CSV results were created under tmp/clp-host. This English report is published on the separate report-only branch. The supplied ZIP contains this report, reproducible host sources and results, and upstream LICENSE/NOTICE when available. Host adaptations are marked as diagnostic, modified source and are not official firmware.

One local patch application initially failed because a delete and add targeted the same path in one patch; it made no changes. It was corrected with an update patch.

## Next action

Use the owner's planned saved project folder to inspect exact per-kit FX settings and relevant voice/modulation/routing state. Prioritize the three differing rows; no further manual threshold sweeps are needed initially. Reproduce a real saved case before assessing a signal-dependent feedback reduction. This comparison establishes a starting reference, not readiness for a new firmware image.
