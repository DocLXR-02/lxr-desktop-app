# Euclidean length regression

Run from the repository root:

```sh
python3 tests/euclidean/run.py
```

Requires Python 3 and a native GCC-compatible C compiler with ASan/UBSan (`CC`
selects the compiler). No firmware or hardware executes. Temporary build outputs
are automatically removed outside the repository. The candidate generator uses
the original, unshimmed CMSIS intrinsic and its production zero guard.

The fixture extracts original function bodies at runtime with source line
annotations. HAL, audio output, display/routing and FatFs have bounded host seams.
The normal named-file and autosave wrappers execute against an in-memory byte
stream. No SD card, target interrupts or full clock/song engine is simulated.

The six groups cover:

- 2,176 exact rhythm masks for lengths 1–16, pulses 1–length, rotation inputs
  0–15 (including the original retained-rotation behavior for rejected inputs).
  The pinned SHA-256 oracle comes from `lab-setup` commit
  `5829398cea5d337200393757e39a37d4d8148256`, with ARM CLZ(0)=32 emulation only
  for that old baseline. To reproduce it where the base Git object is available,
  run `python3 tests/euclidean/run.py --record-baseline`. Normal tests do not need
  Git history or networking. Review the oracle before changing it.
- All byte-valued track/pattern/length boundary inputs, literal lengths 1–64,
  legacy zero decoding, and invalid setter rejection.
- Playback next/previous wrap arithmetic across all lengths, flam 0–5, all
  16 encoded shifts, both boundary steps and all 24 sub-ticks; recording at
  every step before/after half-step quantization; mixed longest-track selection.
- Full named PatV03 and temporary pattern save/load/save, including legacy zero,
  all lengths 1–64, payload/mask preservation, exact bytes and layout. Named
  file size stays 3663 with length offset 3655; tmp size stays 3650, offset 3642.
- Every current-format byte truncation, invalid lengths 65–255 on every track,
  I/O errors with full-looking counts, invalid temporary pattern prefixes, and
  synthetic V1/V2 length tails. The loader installs no partial length/kit tail.
  A wholly absent V1 length tail defaults to 16; partial tails fail. V2/V3/tmp
  require all seven lengths and the kit byte. Real historical files still need
  separate compatibility validation.
- All byte-valued internal SysEx lengths across 448 legal slots, extra bytes,
  every counter value 448–65535, and mode restart. Invalid length values consume
  their slot without writing; counters stop advancing at the end of the stream.

After building Release and Debug, validate the local images without uploading:

```sh
python3 tests/euclidean/check_images.py build/release build/debug
```

Use `--objdump /path/to/arm-none-eabi-objdump` when absent from PATH. This checks
header/magic, payload length, checksum, raw-bin identity, vectors, allocated ELF
section bounds, load addresses and minimum stack space. Linker checks remain
active. Static memory checks do not establish worst-case runtime stack use or
real-time audio performance. Do not flash or publish binaries through these tests.

Both scripts stop on any unexpected compiler, sanitizer, assertion or image error.
The host fixture is source regression coverage, not an equivalence claim for
public official v1.71 or private support firmware.
