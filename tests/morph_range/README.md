# Morph 0–127 regression tests

Run `python3 tests/morph_range/run.py` from the repository root. Requires the
baseline Git object 5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527, native GCC and
ASan/UBSan. No hardware, physical card, audio execution or flashing is involved.

The runner extracts actual production interpolation, Morph tick/request, UI
clamping/input/display and held-step restoration function bodies. It includes
the actual datatype, page and automation-target tables. Unrelated UI, sound
engine, MIDI parser and HAL effects are mocks. The storage executable uses the
existing project_storage fixture with production serializers/readers, initKit
data and a mocked filesystem. These are extracted-function tests, not native
builds of the complete firmware translation units; ARM builds cover the latter.

Coverage:

- Every 8-bit source/destination pair at all 128 legal amounts: exact endpoints,
  monotonicity, bounds and at most one output LSB deviation from ideal rational
  interpolation. Every pair is also tested at invalid amounts 128–255.
- Both Morph UI pages, both normal knobs and encoder edit mode, both display
  layouts, all legal values and 8,000 repeated outward endpoint movements.
- All 128 real non-identical-kit Morph sweeps, excluding the existing four
  transport slots and retaining the existing routing exceptions.
- Actual automation target exclusion and 256 held-step edit/disarm restorations;
  no new automation or MIDI mapping is introduced.
- RAM and SD Morph Kit save/load at all 128 amounts, exact destination, existing
  modulation-target exceptions and unchanged 255-byte named-kit layout.
- All 256 runtime amount bytes absent from temporary kit persistence; existing
  sound bytes above 127 remain intact; RAM source/destination selection retains
  the runtime amount. The amount is outside both sound and global serialization
  intervals, so supported legacy files require no numeric migration.
- Static equality of the menu source except the single datatype change, and of
  relevant serializers/readers against the approved baseline.

Invalid direct runtime requests saturate to 127. This is defensive handling of
an internal byte, not reinterpretation of a stored legacy Morph field: no such
field exists in the supported formats. Old baked Morph Kits remain ordinary
sound data. Existing sound values are never globally clamped or rescaled.

The save_errors oracle retains independent, hash-checked original interpolation
and writer bodies. Equivalent fractional weights still produce byte-identical
payloads; different UI positions now intentionally address different weights.
The new maximum reaches the destination exactly, subject to the pre-existing
non-morphable routing/target exceptions.

ASan/UBSan failures are fatal; LeakSanitizer is disabled for the traced executor.
Each native executable has a 90-second deadline. Actual audio clicks, encoder
electrical behavior, SD latency and hardware timing remain unmeasured.
