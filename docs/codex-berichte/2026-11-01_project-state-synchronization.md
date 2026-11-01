# Project-state documentation synchronization

Date:2026-11-01 (Europe/Vienna). Synchronization ID:LXR-SYNC-2026-11-01-01.

Prepared by ChatGPT in the current conversation after the owner's explicit request to synchronize the knowledge archive and GitHub. This is not a newly started separate Codex Cloud task, firmware release or implementation report.

Repository:DocLXR-02/lxr-desktop-app. New documentation branch:docs/lab-project-state. Exact parent/base:5829398cea5d337200393757e39a37d4d8148256 (historical lab-setup). The commit containing this report is the publication head; its verified SHA is recorded in the owner handoff/private KnowledgeBase after publication. No self-referential commit hash is fabricated inside the report.

## Scope and changes

- Add a current entry point, accepted behavior/scope and open-gate table; link immutable final R11, audit, evidence, measurement design and addendum.
- Continue public changelog IDs after LXR-004 through LXR-011 for implemented CLP/control/arithmetic changes, with design versus fixes distinguished. Retain the old R4 changelog at its original commit.
- Add owner hardware observations without inventing per-test dates or all-voice device coverage. Preserve R4 detailed validation byte-for-byte.
- Update the authored resource-cost ledger and structured release table, keeping R6/R7 alternative ancestry and matched-toolchain boundaries explicit.
- Record separate encoder research status; refresh its private not-started task context because R11 is no longer blocked. No input audit or code fix performed.
- Add continuation/build-report/two-pass rules to this documentation branch's instructions. Root README points to current state. Other source branches do not automatically inherit these docs.
- Update existing private KnowledgeBase MD/ZIP and resource/encoder packages through their existing Library identities.

Production source, tests, CMake, linker, startup, workflows, image and package bytes are unchanged. No existing branch head is intentionally advanced; only the new documentation branch is published. No merge, force-push, release, flash or bootloader action. No new build/test/diagnostic task is started.

## Actual verification

Read the supplied KnowledgeBase1.2.0 and live GitHub branch/tree inventory; read the final R11 report and new audit/addendum documents, R4 changelog/hardware record and applicable AGENTS.md. Existing CI41214348060 was rechecked completed/success at exact source64263ed25f2e9c7f5d880b493d943bfba29271a8; no CI was rerun.

The supplied official1.71 image hash/length, current R11 image/ZIP hash/length, retrieved report Git-blob identities, local document links, JSON values and ZIP structure were checked as document/artifact consistency checks. This is not firmware regression testing or a new binary-resource analysis. Prior native and ARM-probe tests are attributed to their reports, not claimed as reruns here.

Final publication verification compares changed paths to the documentation-only allowlist, retrieves the new status file, and checks all pre-existing heads against the recorded snapshot. Its successful result is reported only after those checks. Library writes are inspected individually; failure or uncertainty must be reported instead of silently claiming synchronization.

## Remaining decisions

R11 remains experimental and not frozen. Owner confirms intended solo MIX/VOL with all-six-FILTER_OFF conditions. Multi-route independent balancing is not implemented. Existing source/numeric faults remain, actual render deadlines/stack high-water/thermal/long-run reliability are unmeasured, and no added processor count is approved. The next task must be small and explicitly scoped; this handoff does not start it.

Start future work at [PROJECT_STATUS.md](../PROJECT_STATUS.md) on docs/lab-project-state, with the pinned firmware source, not this branch's inherited historical code.
