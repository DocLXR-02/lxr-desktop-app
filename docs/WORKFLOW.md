# LXR Firmware Lab — continuation and build evidence

## Intermediate release checkpoints (M4)

**Status:** Documentation-only protocol approved 2026-11-15. It adds a continuity record. The later M5 governance decision permits bounded CHAT-first execution only where source-SHA instructions are compatible; otherwise STOP and seek a separate resolution. Owner authorization remains mandatory for code, CI, firmware, hardware, settings and release actions.

For **each newly authorized release**, create `docs/checkpoints/Rxx.md` only when the release is actually opened. Do not manufacture a checkpoint for already accepted R22 or retroactively rewrite historical milestones. A checkpoint is a short **navigation and state-transition record**, not a substitute for immutable reports, raw evidence, release identities, hardware observations or the resource ledger.

**Required fields:** release ID and fixed topic; state and timestamp; explicit owner authorization/scope; exact base SHA and working branch/head SHA; allowed files/actions and exclusions; reused evidence and pinned reports; latest verified tests/CI; binary/hash only if actually built; owner hardware observations distinguished from host/CI; known FAIL/STOP/NOT ARCHIVED/UNMEASURED boundaries; next blocker and owner decision.

**States:** `OPEN` (authorized task created), `CANDIDATE` (verified build candidate), `OWNER TEST` (awaiting/under owner device evaluation), `STABLE` (explicit owner acceptance), `CLOSED` (no further changes in this release). `STOP`, `REJECTED` and `ARCHIVED` are explicit exceptional/disposition states. Never silently relabel FAIL as PASS. State transitions require evidence and applicable owner approval; documentation is not permission to build, flash or publish.

**Update triggers:** initial authorized release gate, meaningful source/test/CI change, failed stop, candidate handoff, owner test outcome, explicit stable/closed decision, or cross-chat/Codex handoff. Preserve prior decisions and immutable report SHAs; append dated transitions rather than erasing failed history. Do not rewrite the entire KnowledgeBase at each checkpoint.

**Resume gate:** read the current private KnowledgeBase, PROJECT_STATUS, WORKFLOW, EVIDENCE_INDEX, GOVERNANCE_INVENTORY, checkpoint and applicable source-SHA AGENTS.md; independently recheck remote refs and CI. If stale, mark `SYNC_PENDING` instead of guessing. Never convert owner observations to CPU/stack/timing measurements.

**Template (create only for an actually authorized release):**

```text
# Rxx — <fixed topic>
State: OPEN | CANDIDATE | OWNER TEST | STABLE | CLOSED | STOP | REJECTED | ARCHIVED
Last verified: YYYY-MM-DD UTC
Authorization: <owner decision and bounded scope>
Source base SHA: <40 hex>; working branch/head: <branch / 40 hex>
Allowed changes: <paths/actions>; exclusions: <paths/actions>
Reused evidence: <pinned report SHAs, artifacts and limitations>
Host tests / CI: <run, source SHA, result, or NOT RUN>
Build / IMG / ZIP: <exact filenames, sizes, hashes, or NOT BUILT>
Owner hardware: <bounded observation, or NOT TESTED>
Failures and limits: <STOP, NOT ARCHIVED, UNMEASURED>
Next blocker and required decision: <explicit>
Transitions: <dated append-only event references>
```

**Historical M3/M4 boundary:** those implementation commits were documentation-only. The subsequent separately approved M5/M6 governance and KnowledgeBase synchronization is recorded independently; no firmware, tests, CI, settings or accepted source were changed by M3/M4.


Snapshot: 2026-11-15 (Europe/Vienna). Synchronization ID: **LXR-SYNC-2026-11-15-R22-STABLE**. **R22 TEST03 is current owner-approved stable**, exact source `0e37942250f79306627702061d851731bdfdbbc8`, [release record](releases/2026-11-15_r22_owner_accepted_stable.md). R20 is the previous stable source; R18 CLP sound remains a closed protected musical reference. The M5 CHAT-first execution policy below was approved for governance synchronization on 2026-11-15. It does not retroactively authorize any firmware, test, CI, build or hardware action.

## CHAT-first execution and conditional Codex efficiency gate

- **CHAT is the default** for planning, source/evidence review, documentation and narrowly scoped, explicitly owner-approved GitHub code edits where the applicable source instructions and available verification tools permit. A request to review or propose changes is not implementation authorization. For a small/medium authorized change, use an exact source SHA and a separate branch, independently inspect the diff and run essential available regressions/ARM CI where required. Do not imply a test or build occurred if it did not.
- **CODEX is conditional, not automatic.** Recommend it when multi-file complexity, iterative debugging or local build environments materially exceed practical CHAT/GitHub/CI capabilities. Before delegation, provide the six-part efficiency gate below and obtain task-specific owner approval. Codex tasks remain bounded, auditable and independently reviewed.
- **Existing source-SHA instructions remain a separate compatibility gate.** The R22 source `AGENTS.md` still references `lab-setup`, conflicting with the exact pinned-source branching rule. This documentation change does not edit that file or silently waive it. If instructions cannot be reconciled for a proposed task, **STOP** and obtain an explicit owner-approved source-instruction resolution before code editing. Do not branch firmware from the documentation branch.
- **WORK** is reserved for valuable autonomous multi-app/computer workflows, with owner approval and a clear advantage over CHAT. No unapproved agent use.
- **Risk-based validation:** Documentation-only edits require diff, links, integrity and SHA verification, no firmware build. Non-audio UI/logic changes require relevant host regressions and an ARM build when they change firmware; owner hardware checks are explicit where behavior is device-visible. Storage, MIDI, concurrency and real-time DSP changes require focused failure/regression coverage, resource checks, ARM evidence, and owner hardware/audio acceptance where applicable. Never claim CPU/WCET, IRQ/DMA or stack high-water without measurements. STOP on unverified safety-critical behavior or conflicting source instructions.
- **Approval boundaries:** Every code edit, build, physical test, flash, release and sound change requires explicit owner approval. No force-push, merge to `main`, branch deletion, bootloader edit or accepted-reference modification without separate approval. Preserve R4, R11, R18, R20 and R22 reference identities.

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
Use the explicitly approved execution path (CHAT/GitHub/CI for bounded compatible tasks, or Codex after the efficiency gate) for implementation and executable checks. Identify reused evidence.
Objective: <one bounded goal>.
Allowed changes: <explicit scope>; exclusions: <explicit boundaries>.
Reuse previous evidence before new tests/builds. Retain failures and substitutions.
No main/R4 changes, force-push, merge, release, flash or bootloader operation.
Report actual results, unmeasured gates, resource deltas and next decision.
```

This preamble is a template, not an issued task. R18 is **musically closed** by the owner; absent hardware instrumentation, CPU/stack/deadline metrics remain UNMEASURED. Do not silently turn the missing measurements into an unapproved sound change. Assign the next new firmware topic its sequential release number and select its source ancestry intentionally. Future source branches do not automatically inherit instructions from this documentation branch; include the current-state reference in explicit task context.

Firmware delivery: LXR_V2_R<N>.zip contains only LXR_V2_R<N>.img; evidence is separate. Startup line1: LXR LAB R<N>; line2 blank. Do not issue a new image merely to synchronize documents.
