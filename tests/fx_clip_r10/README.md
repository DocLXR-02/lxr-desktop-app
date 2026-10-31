# R10 shared CLP maximum-fader prototype

Exact base: R9 `e2d7963d97cd2d5918e3b80b88156fbbfaa60f47`.
R9 solo sound/response is owner-approved on hardware. R10 is experimental,
not hardware validated, and not a frozen or finalized development stage.
Installation/listening requires separate approval.

Run `python3 tests/fx_clip_r10/run.py --output build/r10-host` with native GCC,
NumPy 2.3.5 and ASan/UBSan/bounds/float-cast-overflow sanitizers.

Production changes only FxBus_clipFaderGains and splash. The helper copies each
configured FX-routed voice's actual OnePoleFilter into automatic local storage,
replays calcOnePole privately once per frame, clamps squared values to [0,1],
then maps their maximum once with the unchanged R9 curve. Exactly one voice
matches R9. Zero routes explicitly return NULL/unity. No activity detection,
summed-fader rule, persistent state, second live update or history reset is added.
A silent/muted/velocity-zero configured voice counts by position. A silent high
fader can hold the whole shared core open. This is not independent per-voice wet
mixing. Route changes are immediate and may step; no crossfade is added.

Mixer capture, gain buffer and actual DSP are byte-identical to R9. Gain touches
only audible lastOut, outside feedback, before unchanged bass/dry/wet arithmetic.
Bass, trim/voicing, parameters, detector, short timing and other effects stay R9.

Tests compile actual production DSP/helper/pole and the verbatim mixer capture
function with six filter-state objects. Pinned R9 helper/map bodies are extracted
with symbols renamed for direct solo reference. Native fixtures do not run the
entire voice engine; the ARM build compiles full integration.

The settled gain examples, knee/bounds/near-zero mapping, all 64 masks x six bus
output destinations, all six solo cases, both existing rates, complete live-filter
identity, no-route fallback, FX/kit resets/re-entry and fresh routing are checked.
Every mask's metadata is compared across 1/7/64 partitions and target events that
exercise maximum-dominance changes. Guard words and sanitizers cover buffers.
No inherited R8/R9 test file is changed. Curve/routing-specific expectations are
explicitly updated only in new R10 fixtures; unrelated component assertions run
unmodified. Static checks allow only helper and splash changes in production.

Pinned R9/R10 solo renders cover kicks, tones, natural/sparse/returning notes,
squared input sweeps, mono/asymmetric stereo, unity and VOL/D/W endpoints.
multivoice.py/c use the actual production R10 helper, pinned R9 helper and actual
mixer adder with synthetic six-voice sources and fixed pans. They check unequal
faders, collective fades/returns, silent high/low/zero routes, alternating notes,
six-voice/hot mixes and route changes at two control sets, mono/stereo. Complete
DSP objects are compared per frame against R9 and a unity-output reference;
metadata, audible metrics and internal invariance are reported separately.
The fixture updates targets every 64 frames; it does not emulate ADC hysteresis
or actual kit synthesis. All DSP waveforms are repartitioned 1/7/64 frames.

Synthetic RMS need not be strictly monotonic and sample steps do not prove
click-free musical behavior. Owner listening must judge the shared interaction
and silent-high-fader compromise. Memory fit/host time do not prove device CPU,
stack high-water or deadline capacity. CI makes one Release ARM build, captures
full source/build evidence and disassembly, and packages the image-only ZIP.
