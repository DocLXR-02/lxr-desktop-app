# LXR Firmware Lab — continuation and build evidence

Snapshot: 2026-11-01. This is a documentation synchronization, not a new development authorization.

## Evidence retention and reproducibility

Follow [RECORDKEEPING_STANDARD.md](RECORDKEEPING_STANDARD.md) for the task report, raw decisive failures, reproducible commands/scripts, test outputs, run manifests, expiring GitHub Actions artifacts and explicit NOT ARCHIVED gaps. Technical summaries do not replace underlying evidence; do not rerun historical tests just to regenerate redundant logs. This is a documentation policy, not authorization to alter production source or start an agent task.

## Starting or resuming a task

1. Read [current project state](PROJECT_STATUS.md), applicable AGENTS.md and the exact report relevant to the requested change. Check live heads against the pinned source. Documentation-branch ancestry is not a firmware baseline.
2. Distinguish SOURCE, HOST TEST, CI/BUILD, OWNER HARDWARE, HYPOTHESIS and UNMEASURED. Preserve failed cases and fixture substitutions. Owner listening does not certify runtime reserve.
3. Use a bounded task: exact base SHA, one objective, edit permissions, exclusions, acceptance/stop criteria and expected outputs. Check existing reports/commits/artifacts before repeating an interrupted task.
4. Keep source changes, documentation branches and delivered images distinguishable. Never merge to main, overwrite historical reports, force-push, flash, alter the bootloader or publish a release without separate authorization.
5. Summarize every task in docs/codex-berichte/YYYY-MM-DD_<task>.md with actual commands/results, failed attempts, limits and immutable references. State when work was not started or measurements were not performed.

## Required build ledger

Every new build intended for delivery needs its own report, not just a changelog. Include:

- Exact base/head/branch and toolchain/configuration; CI run and source archive identities.
- ITCM code, DTCM static data and shared stack gap, general SRAM/heap, and complete flash load payload. Initialized run sections also consume flash; do not double-count them.
- Matched before/after deltas; shared code versus per-instance state, histories and scratch; transient stack frame changes where relevant. Report unknown causal attribution rather than dividing whole-build growth arbitrarily.
- Measured CPU/IRQ/DMA deadlines, stack high-water and audio stability, or explicit UNMEASURED. Code size and host duration are not M7 timing.
- Image header/internal checksum, image size/SHA-256, ZIP size/SHA-256 and exact contents. File rename alone changes neither image bytes nor their checksum.
- Declared sound/UX changes, regression boundary and what the owner should compare. No broad acoustic-neutrality claim from a defined-domain arithmetic test.

Use two passes: functional/musical acceptance first; efficiency/robustness review second. This does not require an expensive full-system campaign for every small documentation/UI change. Optimization may alter sound only after the differences and measured benefit have been compared and explicitly approved; preserve the earlier reference.

## Minimal task preamble

```text
Read docs/PROJECT_STATUS.md on docs/lab-project-state and its pinned evidence.
Read applicable AGENTS.md at the actual source base.
Firmware base: 64263ed25f2e9c7f5d880b493d943bfba29271a8 (verify; do not use the docs branch as source).
Objective: <one bounded goal>.
Allowed changes: <explicit scope>; exclusions: <explicit boundaries>.
Reuse previous evidence before new tests/builds. Retain failures and substitutions.
No main/R4 changes, force-push, merge, release, flash or bootloader operation.
Report actual results, unmeasured gates, resource deltas and next decision.
```

This preamble is a template, not an issued task. Future source branches do not automatically inherit instructions from this separate documentation branch; include the current-state reference in their explicit task context.

Firmware delivery: LXR_V2_R<N>.zip contains only LXR_V2_R<N>.img; evidence is separate. Startup line1: LXR LAB R<N>; line2 blank. Do not issue a new image merely to synchronize documents.
