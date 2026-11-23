# Firmware notifier wildcard correction and original CI archive E2E — handoff

**Date:** 2026-11-23
**Owner authorization:** Promote Lab archival audit PR #16, correct firmware notifier, conduct real original-artifact automatic source-to-private-Lab transfer proof.
**Scope:** GitHub Actions evidence infrastructure ONLY. No firmware source edits, host/ARM firmware builds, device actions, flashing, sound changes, release, or main firmware integration.
**Documentation report base:** firmware `lab-setup` SHA `5829398cea5d337200393757e39a37d4d8148256` (separate report-only branch). Firmware `main` before notifier change `5eb33b87b9749ecac92449c9daa74cc284bbaadf`.

## Diagnosis / corrected implementation

Previously the default-branch workflow `.github/workflows/notify-lab-evidence.yml` triggered on completed `R23*` or `LXR*` names only. Therefore the successful `CC0 policy B - host sanitizers and matched ARM R22 comparison` CI run `45216186960` did not notify the private Lab automatically. Its two genuine originals were manually recovered to private archive commit `8a253403e7b2c603d53a41401c1322944fafe436`.

**Owner-approved corrective workflow-only patch:** isolated branch `infra/notify-all-evidence-ci-20261123`, source commit `4f38ebf19d17b9ada0dd13eba251d835d3285c17`, [firmware PR #4](https://github.com/DocLXR-02/lxr-desktop-app/pull/4) merged to firmware-main commit [`f8325390160f133eda9b43f37f9802fae8850dc5`](https://github.com/DocLXR-02/lxr-desktop-app/commit/f8325390160f133eda9b43f37f9802fae8850dc5).

**Exactly one changed file** `.github/workflows/notify-lab-evidence.yml`; YAML `on.workflow_run.workflows` now includes `"*"` and a negative `"!Notify LXR Lab when CI evidence is ready"` self-exclusion. Job `if` redundantly denies its own name; retains both same-repository security guards, source event validation, original evidence-name filter, limited source Actions:read, and cross-repository dispatch only when evidence artifacts exist. No PR code is checked out by the privileged notifier. No firmware build or source edits occurred.

GitHub official documentation confirms `workflow_run.workflows` accepts glob expressions, including `*` and `!`; this is not a speculative GitHub feature. The previous narrow wildcard gate is removed.

## Executed real end-to-end archive verification

**Isolated probe only:** [draft PR #5](https://github.com/DocLXR-02/lxr-desktop-app/pull/5), `probe/ci-archive-e2e-20261123`, commit `6d5e01c89364f2f80ec799fde30efdee93665b57`. Adds only `.github/workflows/ci-archive-e2e-probe-20261123.yml` to the probe branch, not firmware-main. Workflow name `Evidence bridge E2E probe with an arbitrary workflow name` deliberately does not match old `R23*` or `LXR*`.

[Source run `45225288855`](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45225288855) completed **SUCCESS**. It generated two harmless text files (no source checkout) and uploaded exactly one original GitHub Action artifact:
- ID: `14840554139`
- Name: `ci-archive-e2e-original-evidence`
- Original compressed ZIP bytes: `404`
- Original GitHub SHA-256: `4a9f62a15b14bca8f266dd295232aaf63df6bf227409a0ca58712e68ab0a64c3`.
- Source workflow event: `pull_request`, original source repo `DocLXR-02/lxr-desktop-app`, source SHA `6d5e01c89364f2f80ec799fde30efdee93665b57`.

**Automatic private Lab original-object transfer:** The Lab archive bot, with no manually triggered receiver or recovery run for this sample, committed the genuine original ZIP plus provenance/manifest/SHA register under [Lab archive commit `e0ce67601213d9a3c3b6b89c29daa45a58f75f61`](https://github.com/DocLXR-02/lxr-firmware-lab/commit/e0ce67601213d9a3c3b6b89c29daa45a58f75f61), message `archive(ci): retain original evidence from source run 45225288855`. The archive changes were exactly four paths: the new original ZIP under `archive/objects/evidence/CI/runs/45225288855/`, `archive/provenance/CI/runs/45225288855.json`, `archive/ACTIVE_EVIDENCE_MANIFEST.csv`, and `archive/SHA256SUMS.txt`. Independent GitHub reads matched all original artifact IDs/byte length/source SHA/digest.

**Independent binary archive readback:** A separate archived one-shot verifier executed the already approved `archive_original_ci_evidence.py --verify-remote --root target --run-id 45225288855` directly against the remote Git original ZIP bytes, rather than merely trusting the registries. It committed [certificate `archive/provenance/CI/verification/45225288855.json`](https://github.com/DocLXR-02/lxr-firmware-lab/blob/6e01b184498a138ff9e213cbbe91aa048c924bef/archive/provenance/CI/verification/45225288855.json), reporting `PASS_REMOTE_ORIGINAL_BYTES_REHASHED` with the exact source digest and size. Final private archive head [`6e01b184498a138ff9e213cbbe91aa048c924bef`](https://github.com/DocLXR-02/lxr-firmware-lab/commit/6e01b184498a138ff9e213cbbe91aa048c924bef). New archive total: **82 originals, 57,238,052 bytes**.

## Lab audit and remaining gates

Independent twice-daily name-agnostic audit [Lab PR #16](https://github.com/DocLXR-02/lxr-firmware-lab/pull/16) merged to Lab-main `9a5366eff1241557f0ebb2698612894f8df5690a`. Its **12/12 offline unit tests PASS** ([GitHub CI `45222840675`](https://github.com/DocLXR-02/lxr-firmware-lab/actions/runs/45222840675)). Policy window begins `2026-11-24T00:00:00Z` with twice-daily schedule `17 4,16 * * *` UTC. **The audit's live scheduled execution and negative missed-archive recovery are NOT yet tested**, and utility CI workflows with no evidence may require explicit workflow-ID exemption. Do not claim any scheduled audit PASS until separately observed.

The draft probe PR #5 must remain unmerged; it contains no firmware source and is not a firmware candidate. Temporary one-shot archival verification workflow and prior recovery wrappers remain on private archive staging as housekeeping items; remove only following a separate review. No firmware release, flash, device operation, audible sound or DSP acceptance was performed.

**Handoff classification:** **NOTIFIER MAIN CORRECTED / SOURCE-TO-LAB FRESH ORIGINAL E2E PASS / INDEPENDENT REMOTE BINARY SHA256 PASS / SCHEDULED AUDIT LIVE-RUN PENDING**.

**Documentation publishing:** This Markdown report on `docs/ci-notifier-e2e-20261123` based on `lab-setup`, report-only. Do not merge this branch into firmware-main.
