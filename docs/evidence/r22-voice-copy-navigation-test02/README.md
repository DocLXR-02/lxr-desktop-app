# R22 TEST02 primary text evidence

Source `4372e60337bf5ce35d67e8b5eb577baa3fffb962`, direct TEST01 parent `ace7ad95081aac6a015f11b87b6c540ce100b344`.

- `run-ledger.json`, `regression-results.json`: exact local commands/results; `ci-run.json`, `ci-jobs.json`, `ci-artifacts.json`: verified CI identity and retention.
- `CI_*`: unmodified original successful Actions text outputs, including actual compiler/sections and binary manifest. Original binary archive stays in Actions and local deliverables, not public Git.
- `source-diff.patch`, `source-paths.txt`, `unchanged-boundaries.json`, `protected-refs.json`: exact additive change and protected source/ref boundaries.
- `navigation_cases.c`: focused source-connected UI cases copied from source commit. Run `python3 tests/r22_drum_copy/run.py` in that exact checkout; it reuses existing R21/FatFs fixture, not an independent implementation.
- `name-editor.py`, `name-editor-cases.c`: supplementary full production save/load/name handler and renderer extraction; `python3 name-editor.py /path/to/exact/source`. First fixture declaration/include errors retained separately; corrected run passes without storage/engine adapters.
- `verify-delivery.py`, `delivery-verification.json`: independent checks of original CI outputs. Original local paths are recorded intentionally; adjust paths if reproducing on another machine and retain pinned hashes.
- `stack-entry.py`, `stack-entry.json`: narrow asserted Thumb prologue decoder for matched TEST01/02 artifacts; fixed entry frames only, not a runtime stack bound.
- `SHA256SUMS.txt`: text evidence integrity, excluding this checksum list itself.

All hardware, acoustic and runtime timing/high-water results remain NOT RUN / UNMEASURED. No private KnowledgeBase or firmware binaries are committed here. Permanent binary archive: NOT ARCHIVED. See the separate [English build report](../../codex-berichte/2026-11-14_r22-voice-copy-encoder-navigation-test02.md).
