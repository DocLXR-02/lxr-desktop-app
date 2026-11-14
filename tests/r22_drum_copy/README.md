# R22 cross-kit Drum copy

Run `python3 tests/r22_drum_copy/run.py`. The runner reuses the R21 production
panel/menu/preset/load and real-FatFs RAM fixture through its optional extra-case
entry point; it does not clone those adapters. Run `tests/r21_voice_copy/run.py`
separately for inherited same-instrument and non-Drum behavior.

The focused cases cover all nine cross-kit Drum pairs against the independent
production menu field schema (25 copied base fields), exact preservation of every
other kit byte and the loaded source, no engine/FX/SD activity at transfer, all
nine same-kit rejections, invalid cross-type targets, LFO override/conflicts and
all 306 eligible local destination mappings. They exercise project save/reload,
temporary source recovery, held STEP base values, a cursor aliasing the target,
normal/immediate target loads, target-button press/held/release interception,
non-Drum cancellation, Morph aliases/pending sweeps and commit-time invalidation.

The inherited host adapters observe normal engine parameter/assignment application;
they do not establish physical sound, contact timing, target deadlines or a full
audio-render comparison. The production transfer still performs only an inactive
RAM edit and never calls the live LFO assignment path identified by the R22
preflight. Every same-kit transfer is rejected under owner-approved Alternative C.
No engine, modulation, audio-boundary or same-kit synchronization code is changed.

`validate_arm.py` validates the future CI-produced R22 image/header/boot string,
ELF segment equality, ZIP integrity and static memory against the exact matched
R21 baseline. The per-build report records its actual execution and results.
