# FX Clip input-loss control host tests

Run `python3 tests/fx_clip/run.py --output build/fx_clip_results.json` from the repository root.
A native C compiler and Python 3 standard library are sufficient.

The tests compile the actual production Distortion source with host CMSIS shims.
They cover the diode endpoint, cold silence, per-instance state, hold/fade timing,
first usable sample reopening, algorithm re-entry, mono input consumption,
sample-versus-64-frame block invariance and 72 synthetic mono/stereo renders.
The comparison oracle is the pinned R4 source plus only the defined diode endpoint.

Expected audio is unchanged during the tested continuous input, and settles to
quantized silence after input loss. This is not a complete engine render,
a CPU benchmark, proof of click-free transitions or hardware validation.
The input filter and kit format are unchanged. Stereo inactivity is linked.

The host filesystem/MIDI fixtures use a no-op FX-reset request; actual reset and
DSP state are tested here separately. The firmware CI also builds unchanged R4
with the same toolchain for a resource comparison.

