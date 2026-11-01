# LXR Firmware Lab — evidence retention and reproducibility standard

Adopted: 2026-11-01. Scope: future ChatGPT/Codex/GitHub work. This is a **documentation-only policy**. It does not authorize changes to firmware, tests, CI workflows, source branches, hardware or releases.

## Principle

Maintain **both** a concise decision-oriented technical report **and** the primary evidence required to check or repeat consequential results. A summary is an index and interpretation, not a substitute for a reproducible test. More logging is not necessarily more trustworthy: preserve the decisive inputs, commands, outputs, failures and provenance, not indiscriminate transcripts. Never claim that an absent artifact was archived or that a documented command was rerun.

## Canonical locations and identity

1. Current state and accepted decisions: `docs/PROJECT_STATUS.md`; earlier status snapshots are historical.
2. Exact historical report and evidence index: `docs/EVIDENCE_INDEX.md`, with immutable report/commit URLs. Do not link only moving branch heads as historical proof.
3. One self-contained report per task: `docs/codex-berichte/YYYY-MM-DD_<task>.md` under the existing AGENTS.md handoff policy.
4. For nontrivial audits, keep a separate evidence/reproduction companion in the same directory and reproducibility scripts plus compact machine-readable fixtures/results in `docs/evidence/<task-slug>/` **if publishing these is permitted**. Existing source test directories are used only when code/test modification is authorized.
5. Source identity: exact repository/ref/base/head SHA; build identity: toolchain and config, CI run/job/attempt, ELF/MAP/IMG/ZIP hashes and any artifact IDs. A report-only branch is not a firmware source.
6. Large binary evidence: publish to an explicitly durable, accessible owner-approved location and link with SHA-256 and retention/availability. Do not push large binaries, hardware identifiers, proprietary/sensitive data or credentials to public Git without permission. If no durable location is available, record **NOT ARCHIVED** and the retrieval/expiry risk.

## Required task record

Every completed or interrupted consequential task must distinguish **executed**, **inherited**, **proposed**, **failed**, **not run**, and **not measured**. The report contains:

- Task objective and exclusions; requester authorization; source and documentation baselines; output locations; exact file/commit changes or explicit no-change statement.
- Evidence reuse inventory and why new tests are necessary; technical findings and hypotheses clearly separated; alternative decisions and reasons for rejection.
- Environment: compiler/interpreter, versions and relevant flags, test fixture, seed, actual hardware or host/emulator, input file/hash, limitations and substitutions.
- Executed command or exact reproducible invocation, its parameters, exit code, meaningful stdout/stderr (or an accessible unabridged log), test counts and result; **retain failed attempts and corrective reruns**, not only eventual passes.
- Build/report/run identity, sources of measurements, before/after matched comparisons and unmeasured safety/audio/runtime boundaries.
- Risks, outstanding blockers, owner hardware results as separate dated evidence, and one bounded next decision.
- Published URLs and SHA-256 manifest for retained evidence. Explicitly name important files remaining local or expiring.

A compact machine-readable run ledger should map: run ID -> command/arguments -> inputs/hash -> toolchain -> environment/flags -> exit status -> output/hash -> evidence location -> elapsed time if recorded. Record model/reasoning and task allowance before/after only when actually visible from the product; **do not infer token/credit cost from elapsed minutes or CI time**.

## Evidence tiers: proportionate preservation

| Work class | Minimum retained evidence |
|---|---|
| Simple chat discussion or documentation-only edit | Decision/change note; exact edited paths/commit and any verification. No artificial test program or full terminal transcript. |
| Source diagnosis or design audit | Source pin/lines, precise failing/pass fixtures, hypotheses vs proven effects, executed checks, raw decisive failure/output snippets; retain custom probes when they materially support a finding. |
| Nontrivial host/ARM tests, numerical or safety audit | Technical report plus commands/config/seed, reproducibility script(s), machine-readable results and **full decisive failure logs**, successful aggregate result and limitations; trace expected failures separately. |
| Firmware delivery/release | Dedicated build report under WORKFLOW.md with exact source, toolchain, CI jobs, hashes, memory ledger, tests, hardware validation status, soundtrack/behavior changes and reusable build evidence. |
| Physical measurements | Firmware/hash/device setup, workload, method/instrument calibration, timestamps, raw measurements, units, snapshots, thresholds and stop reasons; never relabel simulator data as device measurements. |

Retain full logs where they contain diagnostic detail needed to reconstruct an issue; for repetitive successful cases store concise per-case results or checksums plus a reproducible generator, not gigabytes of redundant console output. Archive original failing fixtures and trace differences. Avoid saving or asking an assistant to reproduce hidden model reasoning; practical evidence is commands, inputs, outputs, diffs, decisions, tool responses and measurements.

## Temporary logs and availability

- GitHub Actions jobs produce inspectable logs automatically, but are **not permanent archives**. GitHub's current default is 90-day retention; the maximum for public repositories is 90 days. Copy durable decisive outputs, hashes and reproducing commands into the report/evidence bundle before expiry; include the CI run URL and attempt.
- Codex Cloud execution/conversation is **not automatically mirrored to GitHub**. A Codex handoff must explicitly publish the approved report/evidence. Saved cloud VM state is transient and is not a durable source-of-truth archive.
- Local `/workspace/` files and their SHA-256 values are not remotely retrievable merely because they are mentioned in a Markdown report. Publish the authorized relevant raw scripts/results or flag them as NOT ARCHIVED.
- Never publish authentication tokens, private keys, personal data, complete environment dumps or private user material. Sanitize logs before public publication; preserve traceability without exposing secrets.

## Quality gate before closing a task

1. Verify every claimed report/evidence URL, branch, commit and SHA; identify anything that exists only locally.
2. Confirm every significant pass/fail/expected-failure has a reproducible invocation and attributable result; verify the relevant raw evidence is retained or clearly marked unavailable.
3. Check no historical evidence was silently overwritten, no new run was conflated with an earlier one, and CI results are not misrepresented as hardware outcomes.
4. Update `docs/EVIDENCE_INDEX.md` and `docs/PROJECT_STATUS.md` only when appropriate and authorized, with exact date/source identity. Report the index gap until a documentation update can be made.
5. For interrupted tasks, inspect GitHub/CI and saved artifacts **before** repeating work. Stop and request an explicit next-task authorization rather than launching speculative rechecks.

## Known pre-existing retention gap

The 2026-11-01 R11 resource audit published detailed Markdown findings, command examples, run summaries and SHA-256 identities. Its evidence appendix explicitly says many custom analysis scripts, JSON records and local logs stayed outside the repository. Those checks are documented, **but their local-only underlying files are not verified durable GitHub assets**. Their hashes alone do not reconstruct the original bytes. Do not declare that gap closed without obtaining and verifying the exact files; otherwise preserve it as a limitation. The prior interrupted R11 implementation also has a recovered build report; this recovery does not reconstruct unrecorded private command history.

## Efficiency gate

Before asking Codex to investigate/build, read the current project status, exact source and this retention standard; reuse existing evidence; define one bounded objective and selective required tests. Generating small run manifests/log files during a necessary test is generally low-cost compared with re-running the test; re-executing prior heavy analyses solely to produce redundant verbose logs is not justified. Prefer automatic capture by test scripts/CI. Preserve sufficient detail once, then refer to it by immutable identity.
