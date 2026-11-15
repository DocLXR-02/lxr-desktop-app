# LXR Firmware Lab — Governance and Branch Inventory

> **AGENTS resolution record (2026-11-15):** The owner approved a documentation-only correction to the source-root `AGENTS.md` on isolated branch `docs/agents-policy-reconcile-r22-2026-11-15`, commit `da8ef26b828697bab16dfc42760896ce1c881410`, based on accepted R22 source `0e37942250f79306627702061d851731bdfdbbc8`. The verified one-file diff replaces mandatory `lab-setup` ancestry with owner-approved exact-source-SHA branching, aligns CHAT/Codex selection and report-branch handling. **R22 source SHA and historical AGENTS remain unchanged.** D01 is **RESOLVED IN A NEW POLICY COMMIT, NOT RETROACTIVELY**; for future tasks explicitly provide this corrected instruction file and the current WORKFLOW, and STOP if the actual source SHA has conflicting inherited instructions. No firmware build, merge into R22, or firmware release is authorized by this record.


> **M5/M6 update (2026-11-15):** The owner approved the bounded CHAT-first governance policy in WORKFLOW.md. The original R22-source AGENTS.md conflict is historically preserved; a corrected, owner-approved policy file exists at commit `da8ef26b828697bab16dfc42760896ce1c881410`. New work must explicitly apply the corrected policy, and STOP if inherited source instructions conflict. Private KnowledgeBase v1.6.0 is archived in Library as a ZIP; Project attachment sync remains PENDING. Historical v1.5.9 remains immutable. No Library deduplication was performed.


**Audit snapshot:** 2026-11-15. **Scope:** M1/M2 read-only audit carried forward into approved M3 documentation consolidation. **Source:** live GitHub refs and owner Library inventory. This file is an index, not a branch deletion plan.

## Current authority

- **Firmware stable:** R22 TEST03, source `release/r22` at `0e37942250f79306627702061d851731bdfdbbc8`. The original image and ZIP hashes are in [RELEASE_MANIFEST.json](RELEASE_MANIFEST.json).
- **Public documentation:** `docs/lab-project-state` at audit base `5ec588146d24c0b51322c21aa7f09244997f2181`; this is not a firmware build source.
- **Default branch:** `main` at `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`, not the current accepted firmware.
- **Historical base:** `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`.
- **R18 accepted FX-CLP sonic reference:** `41b364067bce0d8cde624c184933d9cb88e2725b`, closed and protected by owner policy.
- **Current KnowledgeBase:** private v1.6.0 (owner-confirmed Library upload; Markdown ZIP member SHA-256 `4acbe7ca072e8677c274d5304d6a427052d66085079d7141e81ec3d83a010de3`); v1.5.9 is historical. Project-attached older copies are stale and remain an unresolved synchronization boundary.

## Branch inventory

**132 branch heads** were enumerated and compared to `release/r22` for firmware/work branches or `lab-setup` for report/documentation branches. The complete **132-row head-SHA register** is in the owner-held M1/M2 audit package `LXR_Firmware_Lab_M1_M2_ReadOnly_Audit_2026-11-15.zip` under `BRANCH_INVENTORY.csv`. This repository index intentionally does not pretend the external package has been published in GitHub.

| Prefix | Count | Classification |
|---|---:|---|
| `codex-reports/` | 48 | Historical Codex reports and evidence |
| `reports/` | 23 | Historical reports |
| `docs/` | 5 | Documentation branches |
| `fx/` | 14 | FX experiments and accepted R18 reference |
| `safety/` | 11 | Historical safety work |
| `fix/` | 6 | Corrective work |
| `feature/` | 3 | Feature work |
| `delivery/` | 3 | Historical delivery |
| `test/` | 3 | Test branches |
| `release/` | 2 | R21 and R22 release sources |
| `integration/` | 2 | Integration work |
| `validation/` | 2 | Validation work |
| Other named prefixes and singleton branches | 10 | Baselines, bug, candidate, CI, controls, diagnostics, euclidean, PR, UI |
| **Total** | **132** | **Preserve all** |

**Provenance exceptions:** `codex-reports/2026-10-26-bug001-handoff` diverges from current `lab-setup` at earlier `56593cacc789e4367f7bb7a63ac2511bfdcd1195`. `docs/r4-hardware-validation-2026-10-28` has inherited firmware/test/delivery history even though its head commit changes README only. Historical `delivery/` branches contain unique delivery commits. Neither divergence nor an old branch name implies corruption, rejection, or permission to delete. All 76 report/documentation head commits were path-inspected; this does not prove every historical ancestor is documentation-only.

**Disposition:** `PRESERVE` for all 132 heads. No rename, deletion, squash, merge, force-push, or inferred acceptance. The source of truth for each exact head SHA is the audit CSV and live GitHub.

## Library archive inventory

| Audit measure | Count | Evidence interpretation |
|---|---:|---|
| Library file records | 93 | Metadata inventory |
| Raw-byte accessible files | 50 | SHA-256 verified |
| Accessible ZIP archives | 46 | ZIP CRC PASS, nested ZIPs tested where present |
| Metadata-only files | 43 | 36 images and 7 other records; raw access unavailable, not missing |

The complete 93-row archive inventory, SHA-256 values and access status are in `LIBRARY_INVENTORY.csv` within the owner-held M1/M2 audit package. The original R22 TEST03 IMG and distribution ZIP were byte-verified against CI/Library evidence. R20's original owner-approved distribution ZIP still lacks a confirmed durable raw-file location in the inventory; the CI `NOT_FOR_FLASH` candidate must not be relabelled as approved distribution. The user-uploaded `LXR_V2_V1.7.img` has **UNKNOWN provenance**. Historical R11 raw audit gaps remain **NOT ARCHIVED**.

## Governance discrepancies and open decisions

| ID | Priority | Status | Required decision |
|---|---|---|---|
| D01 | HIGH | RESOLVED FOR NEW TASKS / HISTORICAL SOURCE UNCHANGED | Corrected `AGENTS.md` at `da8ef26b828697bab16dfc42760896ce1c881410`; inherited source-SHA instructions require an explicit compatibility check and STOP on conflict |
| D02 | HIGH | OPEN | GitHub protection/rulesets absent; owner authorization needed for settings |
| D03 | HIGH | OPEN | Locate and hash exact approved R20 distribution ZIP before CI artifact expiry |
| D04 | MEDIUM | ACCESS LIMITED | 43 metadata-only Library records |
| D05 | MEDIUM | UNKNOWN | Uploaded V1.7 IMG is not proven official recovery firmware |
| D06 | MEDIUM | DOCUMENTED | Documentation branch diverges from firmware source |
| D07 | MEDIUM | DOCUMENTED | Report/docs branch head role differs from inherited source history |
| D08 | MEDIUM | INDEXED | 132 branches require retention-aware tracking; full CSV remains owner-held |
| D09 | MEDIUM | NOT ARCHIVED | R11 raw evidence gaps; do not reconstruct as measured |
| D10 | MEDIUM | UNMEASURED | CPU/WCET, IRQ/DMA deadlines, stack high-water and long-run reliability |
| D11 | LOW | DOCUMENTED | Historical release naming and no native GitHub Release |
| D12 | LOW | OPEN | KB historical/current separation and Project attachment synchronization |
| D13 | LOW | PRESERVED | R22 TEST03 STOP remains FAIL despite later PASS |

**Audit package:** `LXR_Firmware_Lab_M1_M2_ReadOnly_Audit_2026-11-15.zip`, SHA-256 `011873657a85c51e02161bc69cb793150e43eb9d71cf3e05f8e8cba1f603c9c7`. This owner-held artifact is not a GitHub repository file. Do not represent this index as a complete embedded branch manifest until the original CSV is explicitly archived in the approved durable location.

## Checkpoints

See [WORKFLOW.md](WORKFLOW.md) for the M4 lifecycle. **No new R22 checkpoint is created retroactively**: R22 is already owner-accepted stable and the original reports are immutable. A future authorized firmware release gets `docs/checkpoints/Rxx.md` at its first approved development gate, updated at each material state transition. Checkpoint status must never override the owner acceptance record or substitute for a per-build report.

## Change boundary

This document records governance and archive state only. No code, firmware, CI workflow, release, hardware test, private Library, Project attachment or GitHub settings change is authorized by this inventory. M5 policy harmonization and M6 Library publication were separately approved and completed. Replacement of older Project attachments is not supported by this Library action and remains SYNC_PENDING; no Library cleanup or branch deletion is authorized.
