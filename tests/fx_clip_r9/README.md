# R9 delayed-onset CLP audible core attenuation

Exact base: R8 `7681da435cb5a70be3b08654227b5183acec1053`.
R9 is experimental and not hardware validated. Owner R8 listening is separate evidence.
Run `python3 tests/fx_clip_r9/run.py --output build/r9-host` with native GCC,
NumPy 2.3.5 and ASan/UBSan/bounds/float-cast-overflow sanitizers.

The only audio change maps each private replayed squared-fader value q:
q <= 0 gives 0; q >= .25 gives 1; otherwise 2*sqrtf(q).
The steady-state knee is half physical travel, not q=.5. No time delay or new
smoother is introduced. Live voice filter ownership, coefficients and cadence,
mixer capture, exactly-one-route eligibility, zero/multiple unity fallback and
policy-driven routing transitions are unchanged from R8. Gain still multiplies
only audible lastOut, never feedback, dry or parallel bass. No bass adjustment.

Tests link actual production FxBus and 1PoleLp, unchanged DSP and pinned R8 sources.
The production mapping is exercised through FxBus_clipFaderGains, not a test copy.
An independent double-precision oracle checks measured gain within float tolerance.
The exact production mixer capture function is extracted into filter-state fixtures
for all six voice objects; this is not a native simulation of the entire engine.
The ARM build compiles the full mixer and all voices.

Explicit curve-specific changes relative to R8 tests: replay output is compared
against the mapped live update; steady q=.25 and .64 now require unity; moving
render gains come from the actual helper, and both references use actual calcOnePole.
No inherited test is edited. The original DSP, state, bus and regression fixtures
remain in use without reduced assertions. Source checks require the DSP/mixer and
all existing production/config files outside the two authorized edits to match R8.

Coverage: physical table, clamps, knee neighbors, smallest positive float,
100001-point monotonic/bounded sweep; untouched complete live filters; all six
trajectories and 1/7/64 partitions; 144000 per-frame complete DSP object comparisons
using actual mapped R9 gain versus R8 squared gain; all 64 route masks, all 36
sole-voice/output routes, silent extra routes, no/multiple fallback, FX/kit reset,
returning notes and route transitions. Reused 216000-step component assertions
protect bass/dry, VOL/D/W, unity and other distortion algorithms.

Paired pinned-R8/R9 renders cover unity, static gains, synthetic kicks, tones,
natural/sparse/returning notes, mono/identical/asymmetric stereo, regeneration,
physical squared-input-plus-output sweeps and output-only controls. Matching full
DSP object fingerprints are checked at block boundaries. Sample peaks, steps,
idle settling, selected 1/7/64 waveform equality, WAVs and all-six-voice gain CSV
are recorded. Real ADC targets are captured per block; render sweeps use synthetic
per-sample targets. Total output RMS need not be monotonic and synthetic results
do not establish musical or device timing success.

The branch-specific CI makes exactly one Release ARM build with the established
14.3.Rel1 toolchain. It archives source, ELF/map, logs, results, tool versions,
disassembly and hashes separately from the image-only package. Memory fit does
not establish CPU, stack or interrupt headroom. Generated code must be reviewed.
