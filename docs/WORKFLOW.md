# LXR Firmware Lab — continuation and build evidence

Snapshot: 2026-11-01. This is a documentation synchronization, not a new development authorization.

## Chat-first execution and mandatory Codex efficiency gate

**Default to normal ChatGPT CHAT** for discussion, source and GitHub read-only analysis, architectural decisions, task planning and independent result reviews. Work is exceptional: recommend it only when autonomous multi-application, cloud-browser or computer interaction has concrete value beyond CHAT and targeted Codex. Use Codex for explicitly authorized implementation, toolchain-dependent builds, executable tests and tightly scoped engineering work. Neither a longer question nor higher reasoning effort alone justifies Work. Projects organize context; they are not an execution mode.

Before **every** Codex handoff, independently inspect the current private KnowledgeBase, this workflow, [PROJECT_STATUS.md](PROJECT_STATUS.md), [EVIDENCE_INDEX.md](EVIDENCE_INDEX.md), applicable AGENTS.md at the **actual source SHA**, and relevant immutable reports, diffs, source code, test fixtures and CI/artifact metadata. Reuse established results; do not rediscover them without a stated gap. Present an explicit efficiency gate to the owner:

1. Why CHAT alone cannot finish the work, and whether Work has any incremental value.
2. Existing evidence to reuse, including open/failed/unmeasured boundaries.
3. Exact source base SHA, narrowly scoped objective, allowed files/actions.
4. Excluded work, safety boundaries, acceptance tests and objective stop criteria.
5. Minimum necessary validation/builds, retained raw evidence and per-build report requirement.
6. Recommended model/reasoning, qualitative workload and owner-reported available usage budget; **unknown** if unavailable. Do not turn elapsed wall time into a fabricated credit/token estimate.

Large tasks at low remaining budget require owner reapproval. Do not silently trade away memory safety, audio regression checks or essential real-device validation for agent savings. Never start a next task automatically after a report. The owner controls code edits, hardware flashing, releases and sound-altering optimization. Preserve main, R4 and R11 reference source and artifacts.

After completion, verify the pushed report, actual commit/diff, CI and build/test evidence, retained raw reproductions and unchanged protected refs. A streaming interruption is not evidence that execution rolled back: inspect remote output before restarting. Keep each delivered firmware image tied to a distinct build report. New chats resume from the current private KnowledgeBase, pinned source and GitHub docs, not from reconstructing an entire chat transcript. Model-specific product limits and currently selected thinking effort are not reliably observable from source control or by this assistant.

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
Read docs/PROJECT_STATUS.md, docs/EVIDENCE_INDEX.md, docs/WORKFLOW.md and docs/RECORDKEEPING_STANDARD.md on docs/lab-project-state and their pinned evidence.
Read applicable AGENTS.md at the actual source base.
Firmware base: 64263ed25f2e9c7f5d880b493d943bfba29271a8 (verify; do not use the docs branch as source).
Only if CHAT is insufficient: explain the execution need and identify reused evidence.
Objective: <one bounded goal>.
Allowed changes: <explicit scope>; exclusions: <explicit boundaries>.
Reuse previous evidence before new tests/builds. Retain failures and substitutions.
No main/R4 changes, force-push, merge, release, flash or bootloader operation.
Report actual results, unmeasured gates, resource deltas and next decision.
```

This preamble is a template, not an issued task. Future source branches do not automatically inherit instructions from this separate documentation branch; include the current-state reference in their explicit task context.

Firmware delivery: LXR_V2_R<N>.zip contains only LXR_V2_R<N>.img; evidence is separate. Startup line1: LXR LAB R<N>; line2 blank. Do not issue a new image merely to synchronize documents.
