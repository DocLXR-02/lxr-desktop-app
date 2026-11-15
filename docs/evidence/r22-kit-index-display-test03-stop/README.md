# R22 TEST03 stop evidence

The source branch remains at TEST02 `4372e60337bf5ce35d67e8b5eb577baa3fffb962`. No candidate was built. This is retained failing draft evidence, not a passing implementation.

- `draft.patch`, `draft-paths.txt`: full six-file uncommitted change against exact TEST02 base; only production change removes the display offset.
- `navigation_cases.c`: exact failing draft oracle, retaining the assertion at line 253.
- `r22.log`: unabridged failing suite traceback/SIGABRT. Buffered stdout was not flushed before abort; no invented PASS transcript.
- `regression-results.json`, `regression-driver.log`, `run-regressions.py`, `run-ledger.json`: exact commands, results and stop state. Other `.log` files are actual completed independent host checks.
- `pinned-source-excerpts.txt`, `unchanged-boundaries.json`: independent source explanation for invalid zero LFO voice IDs versus normal load normalization, and unchanged production modules.
- `reused-test01-ci.json`, `reused-test02-ci.json`: successful existing CI run identities; no new TEST03 CI exists.
- `protected-refs.json`: all existing heads unchanged after stop. Publishing adds only a new report branch.
- `SHA256SUMS.txt`: integrity of evidence excluding the checksum file itself.

To reproduce the retained failure in a disposable checkout, use the exact base, apply `draft.patch`, then run `python3 tests/r22_drum_copy/run.py`. This invokes the inherited ASan/UBSan production/FatFs fixture. It is expected to fail at the retained whole-active-buffer assertion; do not claim the incomplete 448-case oracle passes. Do not apply this patch to a protected working tree or publish it as a candidate without an explicit resume decision. The normal load path converts invalid LFO voice IDs to 1; a fixture-only setup correction is proposed in the report but was not applied or rerun.

No hardware, flashing, merge, release or private KnowledgeBase publication. No TEST03 IMG/ZIP/ELF/MAP exists. Permanent binary archive: NOT ARCHIVED. See the [English stop report](../../codex-berichte/2026-11-15_r22-kit-index-display-test03-stop.md).
