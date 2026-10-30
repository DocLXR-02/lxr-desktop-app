# Experimental R7: trimmed R5 core plus parallel bass

Base is exact R5 `af3562fece090288a83727524c1035b79f4cf20b`, not R6.
Run `python3 tests/fx_clip_r7/run.py --output build/r7-host` with native GCC
sanitizers and NumPy 2.3.5. CI builds Release once with the recorded R5 Arm
GNU 14.3.Rel1 version, and packages image and source/build evidence separately.

R5's entire CLP core is retained with only floating excitation trim 0.22.
The original bus drives a separate first-order LP with alpha
`.021125671293435455134` (150 Hz pole at 44140 Hz). Its contribution is
`state * -0.25 * vol`, added to the wet output outside lastOut/feedback, before
the existing dry/wet blend. Negative polarity was selected from host comparisons
of weights 0, +/-.125, +/-.25 and +/-.30: positive addition cancelled bass in
several probes. Phase effects remain signal/control dependent, requiring listening.
The magnitude .25 gives a wet bound .647479+.25=.897479 under normal controls;
no added limiter, compression, automatic normalization or wet makeup is used.

Two new double LP histories initialize normally and clear at the existing
audio-owned CLP entry after queued resets. The original bus/dry samples are
never changed. Detection remains before any trim/quantization, with R5's linked
stereo, 441-frame hold and float fade (zero on subtraction 222). No long release
or R6 split input HP is included. Existing controls and kit bytes are retained;
the parallel contribution follows vol and bypasses the core ton filter.

The suite compiles actual production R5/R7 source. A third reference is actual
R5 with only the same floating input trim inserted. R7's core-return trace at
block boundaries must match that reference in every principal render; the
parallel weight variants must also preserve that trace. Static recovery checks
that removing the documented additions exactly restores R5's process body.

Unmodified inherited state and six-route bus fixtures still run. New checks
cover init/reset, input preservation, detector reopening through hold/fade,
zero volume, zero wet, seeded return-only core, Tube/Fold identity, per-sample
headroom, finite state, guards and 1/7/64 block partition consistency.
318 principal R5/R7 pairs plus trimmed-core references cover kicks, tones,
mixed tones, natural quantized decay, sparse/returning notes, squared fader
levels, input loss, DC endpoints, impulses, maximum regeneration, mono and
asymmetric stereo. The old R5-vs-bounded-R4 continuous identity render is not
an R7 timbre criterion; no inherited assertion has been weakened.

JSON records 30–150 Hz / 500–5000 Hz Hann-windowed RMS over first 480 ms;
`body_` metrics use first 180 ms to include the kick body with less tail weighting.
Peaks and maximum steps are raw LSB. Selected unnormalized WAV pairs permit
later listening. These are synthetic probes, not instrument recordings, proof
of musical success, click-free re-entry or real-time CPU headroom.

The image-only `LXR_V2_R7.zip` contains exactly `LXR_V2_R7.img`.
`package.py` verifies header/checksum/vectors, application flash ranges, and
image/ELF bytes. R7 is experimental and not hardware validated.
