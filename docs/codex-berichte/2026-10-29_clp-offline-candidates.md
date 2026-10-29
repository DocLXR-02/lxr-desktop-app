# FX CLP offline input-loss suppression experiments

Date: 2026-10-29 (Europe/Vienna)
Repository: DocLXR-02/lxr-desktop-app.
Reference source: R4 c870f7249d7ee0d65423bc8b7bfdb422dcc74482.
Report branch: codex-reports/2026-10-29-clp-offline-candidates
Report branch base: lab-setup 5829398cea5d337200393757e39a37d4d8148256.
Head: the commit containing this report.

## Objective and authorization boundary

Continue the CLP investigation autonomously after the CLP-1 data audit. Compare offline mechanisms that stop feedback when bus input disappears while preserving the useful input-driven sound. This task changes diagnostic host sources only. It does not modify firmware, produce a firmware image, flash hardware, or declare a hardware fix.

The supplied authoritative cases and Kit 4's history dependence are recorded in docs/codex-berichte/2026-10-29_clp1-project-audit.md on its report branch. The previous bounded host reference reproduces those measured threshold cases under the specified protocols.

## Diagnostic reference and changes

The full original Distortion_process implementation is reused through host-stub intrinsics. The previously documented positive diode-table endpoint guard remains in all reference/candidate runs. Thus "reference" here means the bounded host reference, not bit-identical original firmware.

Three actual feedback expressions are modified in the experimental copy only:
```c
signal += dist->lastOut[channel] * dist->p1 * 0.125 * host_gate;
```
The additional multiplier affects returned feedback, not dry/wet mix, input gain, tone coefficients, output volume, or the diode curve. No input-filter coefficient is changed.

Experimental detector state lives outside MasterDistortion for this isolated harness. It is not an integration design: per-instance ownership, initialization, transitions between FX types, parameter changes, automation and resource use still need design and verification.

## Candidate definitions

All detectors inspect max(abs(left_input), abs(right_input)) before the FX. They do not inspect wet output, so the oscillator cannot hold its own detector open.

0. Bounded reference: feedback multiplier 1.
1. Exact-zero inactivity: any nonzero input immediately restores multiplier 1. After 441 consecutive zero samples (approximately 10 ms), multiplier falls linearly to zero over 221 samples (approximately 5 ms).
2. Peak-envelope scaling: envelope=max(abs(input), envelope*0.998867882756331), approximately 20 ms decay. Multiplier=min(1,envelope/16), where amplitude is in int16 least-significant-bit units.
3. Refined inactivity: same hold and ramp as variant 1, but amplitudes <=1 LSB count as inactive.
4. Feedback-off control: multiplier 0, used to distinguish amplified low-level input from autonomous oscillation.

Variant 3 immediately restores full feedback for input magnitude >1 LSB. Detectors are initially open in this experiment to model adding control to a running effect. A production boot/FX-entry initialization policy was NOT verified. A newly enabled effect starting with no input must not be assumed transient-free from this test.

The 1-LSB tolerance, 10-ms hold and 5-ms ramp are experimental constants, not validated final user-facing settings. No additional UI or preset field is selected.

## Test signals and parameter rows

390 complete stereo renders, each 4 seconds at 44140 Hz:
13 parameter/state rows * 6 input scenarios * 5 variants.

Rows 0-9 use the corrected CLP-1 controls, except row 4 uses col=7 for guaranteed onset. Rows 10 and 11 stress maximum drv/col/vol at ton=127 and ton=0. Row 12 models the Kit 4 sustaining state: col=7 initially, changed to col=6 at 1 second, preserving DSP state.

Signals:
- Continuous sine, 63 Hz, amplitude 12000 integer units.
- Synthetic decaying sine kick bursts at 160 BPM, approximately 0.25 s duration, followed by a long gap and return.
- Same kick sequence with deterministic +/-1-LSB residual noise in the long gap.
- Quiet continuous sine, 20 Hz, amplitude 4 integer units.
- Continuous sine until an abrupt input cut at 1.5 seconds.
- Different left/right sines; right input stops at 1.5 seconds while left remains active.

These are synthetic diagnostic inputs, NOT the actual saved Voice 1 rendered through the complete firmware. Physical fader smoothing, actual envelopes and patterns, sample timing, unrelated voices and hardware output are not reproduced by these signals.

DSP calls are one sample at a time in the experiment, unlike normal firmware block processing. Coefficients/states are retained, but CPU cost and prospective block integration cannot be inferred from this harness.

## Results

| Check | Exact-zero inactivity | Peak-envelope scaling | Refined inactivity (<=1 LSB) |
| --- | --- | --- | --- |
| Continuous strong sine, entire render | Sample-identical to reference in all 13 rows | Same | Same |
| Continuous 4-LSB, 20-Hz sine | Sample-identical in all 13 rows | Different in all feedback-enabled rows | Sample-identical in all 13 rows |
| First 200 ms of synthetic kick sequence | Sample-identical in all 13 rows | Same | Same |
| Clean long gap: final measured output | Quantized zero in all rows | Quantized zero in all rows | Quantized zero in all rows |
| Abrupt input loss: worst settling time below 32-LSB peak | 24.8301 ms | 209.6511 ms | 24.8301 ms |
| +/-1-LSB residual input, worst late RMS | 0.25614 full-scale | 0.21625 full-scale | 0.001799 full-scale |
| Late residual-input output versus feedback-off control | Not generally equal | Not generally equal | Sample-identical in all 13 rows |

Settling criterion: time after the 1.5-second input cut until no later sample exceeds 32 LSB (about -60 dBFS peak). This is a measurement threshold, not a detector setting and not a universal inaudibility claim.

Late gap RMS is measured from 2.0 to 2.5 seconds. The remaining RMS in variant 3's noise test is input-driven amplified residual noise: direct sample comparison in that interval gives maximum difference 0 LSB versus the feedback-off control, across all rows. It should not be reported as perfect silence with nonzero input.

Both inactivity variants also quench the sustaining Kit 4 row. The reference remains able to sustain at col=6 after prior excitation; suppressing loop return extinguishes it after input loss.

### Sound preservation boundaries

Variant 3 leaves the first synthetic kick and the tested uninterrupted tones sample-identical. Later kicks after silence do NOT retain sample-identical waveforms: the persistent feedback state has been removed. That is an inherent consequence of stopping the inter-hit oscillator in this experiment, and must not be hidden behind a loudness metric.

For the returning kick, RMS over the first 20 ms differs by at most 0.0946312 dB from reference across the 11 feedback-enabled rows. This supports absence of a large attack-level change in this input set; it does NOT establish perceptual equivalence, unchanged timbre, lack of every click, or behavior with actual saved voices.

The stereo detector is linked: if either channel remains active, feedback remains active for both. Thus the asymmetric-channel case retains original behavior on the silent side while the other side plays. This experiment suppresses feedback when the shared FX input becomes inactive, not independently per channel or per voice. Changing this policy could change stereo sound and requires a separate explicit decision.

Inputs of <=1 LSB for longer than the hold period intentionally do not hold the refined detector open. Arbitrarily quiet or sparse content is therefore not promised to remain unchanged. Higher residual noise can also keep the detector open. These tradeoffs must be evaluated on real full-engine output before selecting a final threshold.

## Recommendation

Advance variant 3, input-side inactivity detection with a small residual tolerance and a smooth reduction of loop return, as the leading offline candidate. It performs better on this test set than proportional envelope feedback scaling, which alters quiet signals, releases more slowly, and can still sustain at extreme gain with residual input.

This recommendation is for the next integration/prototype stage, not approval to ship or flash. No fixed-output gate, compressor, limiter, independent output-control redesign or filter redesign is included.

The next technical work is:
1. Render the actual saved voice and realistic fader/input transitions through the candidate; compare tails, retriggers and stereo routing.
2. Define detector initialization and ownership at FX/kit switches, and assess release/reopening discontinuities.
3. Implement only the selected behavior on an isolated R4-based source branch after firmware changes are authorized; preserve separate identity for the diode correction.
4. Check ARM build, memory and runtime cost, then prepare a clearly identified hardware-test artifact with the existing recovery constraints.

## Commands, artifacts, and limits

Compiler: Ubuntu cc 13.3.0. Builds use:
```sh
cc -std=c11 -O2 -fno-strict-aliasing -fsanitize=undefined,bounds -fno-sanitize-recover=all tmp/clp-candidates/experiments.c -lm -o /tmp/clp_candidates
/tmp/clp_candidates tmp/clp-candidates/renders
/tmp/clp_candidates tmp/clp-candidates/renders 4 5
python3 tmp/clp-candidates/analyse.py
```
The final harness defaults to variants 0-3; arguments 4 5 generate the feedback-off control. All relevant compile/run calls completed with exit 0 and no sanitizer diagnostic. A Python check verified all 390 raw files had the expected full length. Numeric output, code and compact WAV examples are included in the analysis bundle; raw renders are reproducible and omitted to avoid a large delivery.

During execution the cloud environment briefly disconnected after a completed render run. A subsequent shell check reported no usable environment; after the environment returned, existing outputs were inspected and complete lengths verified. No result was inferred from an incomplete render.

Metrics: metrics.csv, summary.json, noise_control.json, return_level.json.
Audio examples: CLP_Synthetic_Kick_Reference.wav and CLP_Synthetic_Kick_Candidate.wav use row 4, d/w=40, same input, sample scale and playback level, without individual normalization. They are clearly synthetic examples, not device recordings or reconstructions of the owner's kick. The reference intentionally contains a sustained tone; audition at low volume.

analyse.py can regenerate the basic metrics after renders exist. The supplemental checks for feedback-off equality and returning-kick level are simple comparisons described above and recorded in their JSON outputs.

No firmware files, main/R4 branch, or owner's project files were changed. This English report alone is published on the report branch; code/audio are diagnostic deliverables. No hardware validation, full-engine fidelity, CPU benchmark, subjective listening validation or final solution is claimed.
