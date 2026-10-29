# Experimental R6 CLP shared-core probe

Base: `af3562fece090288a83727524c1035b79f4cf20b` (R5).
Run `python3 tests/fx_clip_r6/run.py --output build/r6-host`.
Requires native GCC with ASan/UBSan and NumPy 2.3.5 (CI pins this version).

Only the first high-pass is split: the original return keeps its coefficient
`.64116641920131193922`; incoming audio uses `.016936545811725878252`
(120 Hz matched pole at 44140 Hz), then weight `1/32`. The branches sum before
the existing LP1/HP3/LP2, drive, slew/rails, diode, tone and volume stages.
All new bass passes through the shared nonlinear core. No new clean wet bass
branch, fader remap, automatic gain control, UI parameter or kit byte is added.

The two incoming HP histories add 16 runtime bytes. They initialize at init and
clear at the existing audio-owned CLP entry, including queued kit/FX resets.
The detector function, original-bus observation, linked stereo, 441-frame hold
and nominal 221-step fade are unchanged. Float subtraction reaches exact zero
on fade step 222, as in R5. A longer release is outside this experiment.

The suite compiles actual R5 source retrieved from the pinned Git object and
actual R6 production source, with host CMSIS shims. The old state and six-route
bus fixtures are retained. The old `tests/fx_clip/run.py` continuous equality
assertion describes R5 vs bounded R4; it is deliberately not an R6 voicing test.
The new suite compares R5/R6 spectra and peaks rather than requiring CLP audio
identity. Tube/Fold, zero-wet output and seeded return-only CLP remain exact.

264 paired synthetic renders cover four control rows, col=0 probes, squared
fader-equivalent levels, kicks, 60 Hz tones, mixed tones, sparse notes, natural
decay through int16 zero, returning kicks, input loss ramps, alternating full
scale, impulses, mono/identical/asymmetric stereo. Guard words, finite states,
sanitizers, input-loss settling and selected 1/7/64-frame partition checks are
asserted. The input-loss clock follows the actual last nonzero consumed sample,
including the other stereo channel. Band RMS uses a Hann-windowed first 480 ms:
30–150 Hz and 500–5000 Hz. Peaks/steps are unwindowed int16 LSB.

Metrics and selected unnormalized stereo WAVs are evidence for auditioning,
not proof of musical success, an 80–90% physical onset threshold, click-free
re-entry or CPU headroom. No instrument recordings are present. Linked activity
can keep the other side's feedback active; silence requires linked inactivity.

CI builds Release once using Arm GNU 14.3.Rel1, the R5 toolchain version. It
publishes `lxr-r6-image` separately from `lxr-r6-source-build-evidence`.
`package.py` checks the SD container, checksum, vectors, ELF flash ranges and
ELF/payload byte identity. `LXR_V2_R6.zip` contains exactly `LXR_V2_R6.img`.
No bootloader image is delivered. R6 is experimental, not hardware validated.
