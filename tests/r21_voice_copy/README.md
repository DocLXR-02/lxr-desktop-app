# R21 voice copy/paste host fixtures

Run `python3 tests/r21_voice_copy/run.py` from the repository root. A native C
compiler and Python 3 are required. ASan/UBSan execute production transfer,
selector, complete public panel dispatchers, load sweeps, Cortex LFO destination
case, LFO retrigger, and project/FatFs bodies with RAM sectors and explicit
HAL, LCD, unrelated button helpers and parameter-application adapters. The actual
menu table supplies an independent parameter ownership oracle. No hardware runs.

`python3 tests/r21_voice_copy/run.py --morph-alias` is the focused source-sound
preservation gate. At the initial draft it intentionally fails: the stored target
also backs the loaded Morph pointer, and an in-progress sweep reads transferred
bytes. Owner disposition of preserving the current Morph target in separate RAM
versus rejecting that destination is pending. Do not deliver this draft as a
firmware candidate while that gate fails.

The normal fixture checks Kit 10 unsaved Voice 5 edits, a distinct Kit 2,
different Load/Save cursor, exact kit byte ownership for six engines/seven panel
voices, both hi-hat decays, LFO self eligibility and exact-destination conflicts,
project round trips, older short kit compatibility, held STEP base values,
entry/release/bounce/cancel/rejection and commit-boundary clock injection. The
R20 capture/playback/UI, storage/transaction, parser/sweep and FX suites remain
separate regressions. The adapters observe dispatches; they do not establish
physical sound, full engine render behavior, button electrical timing or WCET.

`validate_arm.py` is prepared for the eventual matched CI build. It verifies
ELF/IMG load-segment identity, boot string, updater fields, ZIP contents and
static region spans relative to the exact R20 baseline. It has not been run on
an R21 ARM build at this draft stage.
