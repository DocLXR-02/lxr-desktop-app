# LXR Firmware Lab — continuation and build evidence

Snapshot: 2026-11-14 (Europe/Vienna). Synchronization ID: **LXR-SYNC-2026-11-14-R20**. R18 CLP sound remains the protected musical reference.

## Chat-first execution and mandatory Codex efficiency gate

- **CHAT** handles planning, read-only source and evidence analysis, acceptance criteria, and independent review of Codex results. Authorized documentation-only synchronization belongs in this workflow.
- **A separate Codex Cloud task is mandatory** for every code change, including firmware, test code and build/CI scripts, and for its implementation checks, regressions, tests and builds. This also applies to small fixes. CHAT prepares an English task with the release number, stable topic title, exact source base, authorized scope, tests and expected artifacts. The owner authorizes and launches the task in Codex. If execution starts later, record its task reference and actual status.
- **WORK** is outside the normal firmware workflow. Use it only for a separately agreed application or computer workflow with a concrete need. Allowance planning is based on CHAT and Codex use. Current account limits and usage are unverified.

Before **every** Codex handoff, independently inspect the current private KnowledgeBase, this workflow, [PROJECT_STATUS.md](PROJECT_STATUS.md), [EVIDENCE_INDEX.md](EVIDENCE_INDEX.md), [CLP R18 accepted sound](CLP_R18_SONIC_ACCEPTANCE.md) where relevant, applicable AGENTS.md at the **actual source SHA**, and relevant immutable reports, diffs, source code, test fixtures and CI/artifact metadata. Reuse established results; do not rediscover them without a stated gap. Present an explicit efficiency gate to the owner:

1. The numbered release, fixed topic title, and executable work assigned to Codex.
2. Existing evidence to reuse, including open/failed/unmeasured boundaries.
3. Exact source base SHA, narrowly scoped objective, allowed files/actions.
4. Excluded work, safety boundaries, acceptance tests and objective stop criteria.
5. Minimum necessary validation/builds, retained raw evidence and per-build report requirement.
6. Recommended model/reasoning, qualitative workload and owner-reported available usage budget; **unknown** if unavailable. Do not turn elapsed wall time into a fabricated credit/token estimate.

Large tasks at low remaining budget require owner reapproval. Do not silently trade away memory safety, audio regression checks or essential real-device validation for agent savings. Never start a next task automatically after a report. The owner controls code edits, hardware flashing, releases and sound-altering optimization. Preserve main, R4 and R11 reference source and artifacts.

After completion, verify the pushed report, actual commit/diff, CI and build/test evidence, retained raw reproductions and unchanged protected refs. A streaming interruption is not evidence that execution rolled back: inspect remote output before restarting. Keep each delivered firmware image tied to a distinct build report. New chats resume from the current private KnowledgeBase, pinned source and GitHub docs, not from reconstructing an entire chat transcript. Model-specific product limits and currently selected thinking effort are not reliably observable from source control or by this assistant.

## Numbered releases and branch history

- Assign the next unused sequential firmware release number `R<N>` to a new authorized change before implementation. Use that number as the common identifier in the topic, Codex task, branch, commit/CI records, report and artifacts. Keep one stable topic title such as `R21 - <short scope>`. Change the title only when scope genuinely changes and record the reason under the same release number.
- Prefer one dedicated development branch `release/r<N>` per release from an explicitly pinned source SHA. Keep successive corrections for that release traceable on its branch. Give distinct test images a numbered candidate suffix such as `LXR_V2_R21_TEST01.img` and a matching one-image ZIP. Record each exact commit, CI run and hash. A post-closure change gets a new release number. Release numbering identifies scope and lineage. Final distribution still requires a separate owner decision.
- Use an experimental branch only when an approved comparison or isolation need is written in its task. Include the parent release number and a short experiment identifier, record the fork SHA and disposition, and return the chosen result to the numbered release record. Preserve existing refs and rejected alternatives as history. Source merges, renames of historical refs, and branch deletion require separate authorization.

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
Choose and verify a source base for the particular next issue. Accepted CLP sound reference: 41b364067bce0d8cde624c184933d9cb88e2725b (R18); protected historical R11: 64263ed25f2e9c7f5d880b493d943bfba29271a8. Do NOT assume all branches can be blindly merged or that the documentation branch contains firmware.
Release: R<N>. Topic: <stable title>. Branch: release/r<N>. Base SHA: <exact firmware commit>.
Use the approved Codex task for implementation and its executable checks. Identify reused evidence.
Objective: <one bounded goal>.
Allowed changes: <explicit scope>; exclusions: <explicit boundaries>.
Reuse previous evidence before new tests/builds. Retain failures and substitutions.
No main/R4 changes, force-push, merge, release, flash or bootloader operation.
Report actual results, unmeasured gates, resource deltas and next decision.
```

This preamble is a template, not an issued task. R18 is **musically closed** by the owner; absent hardware instrumentation, CPU/stack/deadline metrics remain UNMEASURED. Do not silently turn the missing measurements into an unapproved sound change. Assign the next new firmware topic its sequential release number and select its source ancestry intentionally. Future source branches do not automatically inherit instructions from this documentation branch; include the current-state reference in explicit task context.

Firmware delivery: LXR_V2_R<N>.zip contains only LXR_V2_R<N>.img; evidence is separate. Startup line1: LXR LAB R<N>; line2 blank. Do not issue a new image merely to synchronize documents.
