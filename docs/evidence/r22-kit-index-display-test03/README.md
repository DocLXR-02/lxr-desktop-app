# R22 TEST03 reproducible build evidence

Source `0e37942250f79306627702061d851731bdfdbbc8`, exact direct TEST02 parent `4372e60337bf5ce35d67e8b5eb577baa3fffb962`. Required local, CI and binary checks PASS. Report branch `reports/r22-test03-2026-11-15` is documentation-only from unchanged lab-setup.

- `preflight.json`: exact live base, unchanged guidance/stop refs and byte-identical recovered draft proof. No private KnowledgeBase content.
- `HISTORIC_STOP_*`: original failed run and its ledgers, copied unchanged from immutable report `bf9162ae0b131c97a12d3ea4ab472a45d96f4cc8`. They remain historical FAIL, not a new resumed test or PASS.
- `authorized-fixture-fix.patch`: only the two lines added to the failed fixture; valid destination LFO IDs 1–6 before snapshots. `navigation_cases.c` and `source-diff.patch` preserve exact final cases/six-path implementation.
- `r22-focused.log`: first resumed test, complete PASS of all inherited cases plus 448 exact label/RAM cases, eight real project/.snd roundtrips and seven added same-kit rejections. Run `python3 tests/r22_drum_copy/run.py` in the exact source checkout.
- `run-regressions.py`, `regression-results.json`, `regression-driver.log`, individual logs: 10 then-executed inherited local regressions, all exit 0; first focused R22 run excluded from redundant local repetition.
- `run-ledger.json`: ordered local/CI/binary provenance, commands and limits. `ci-run.json`, `ci-jobs.json`, `ci-artifacts.json`: completed run 43707621969, attempt 1/job 132792019014 and artifact 14159486119.
- `CI_*`: unmodified original successful Actions text outputs, compiler/section/source manifests and validation.json. The binary archive is in Actions/local deliverables, not public Git. It expires 2026-12-15T11:11:10Z; permanent binary archival NOT ARCHIVED.
- `verify-delivery.py`, `delivery-verification.json`: independently checked original CI archive, exact source, IMG/ZIP/ELF/MAP hashes, updater, boot bytes, ELF load segments and one-IMG ZIP byte identity/CRC. Adjust explicit local paths if reproducing elsewhere; preserve pinned identities.
- `stack-entry.py`, `stack-entry.json`, `stack-entry.log`: reused narrow asserted Thumb prologue comparison for actual matched TEST02/03 artifacts. Fixed entry frames only; excludes later allocation, calls, interrupts and runtime high-water.
- `unchanged-boundaries.json`, `protected-refs.json`, `source-paths.txt`: exact production/ref boundary evidence. All production except the one-line selector display correction stays byte-identical.
- `SHA256SUMS.txt`: integrity of all evidence files except this checksum list itself.

Host ASan/UBSan/real FatFs sector fixtures and exact-source ARM Release success do not constitute physical device tests. CPU/WCET, IRQ/DMA deadlines, stack high-water, long-run reliability and actual audio remain UNMEASURED. No hardware, flash, merge, release or project-wide consolidation. No later task was started. See the [English build report](../../codex-berichte/2026-11-15_r22-kit-index-display-test03.md).
