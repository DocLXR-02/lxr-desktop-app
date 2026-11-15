# R22 TEST03 — Kit index display: stopped before candidate build

Date: 2026-11-15 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**Result: STOP. The required R22 host suite failed in the newly added slot-oracle fixture. No TEST03 firmware candidate, ARM build, source commit or source push was produced.** The one-line display correction and complete test/workflow draft remain available as reproducible text evidence. This report does not label the draft passing or stable.

## Scope, pins and authorization

The owner authorized a presentation-only correction: stored kit indices 0–63 must display K00–K63, consistent with normal LOAD/SAVE and `.snd` prefixes. The owner reported TEST02 cross-Drum copying working on the device; this is bounded owner feedback, not physical acceptance of TEST03 or a complete coverage matrix.

- Source branch: `release/r22`; required base, live prechecked head and unchanged final source head: **`4372e60337bf5ce35d67e8b5eb577baa3fffb962`** (TEST02).
- Accepted TEST01 ancestor: `ace7ad95081aac6a015f11b87b6c540ce100b344`.
- Report-only branch: `reports/r22-test03-stop-2026-11-15`, rooted at unchanged `lab-setup` **`5829398cea5d337200393757e39a37d4d8148256`**. This branch contains documentation and draft text evidence only.
- Guidance refs checked against live heads: `docs/lab-project-state` at `9b68be76d80de50cb84fa9882c5b9003bcb6faf8`; TEST02 report branch at `1559b557db69abd4ef6d152fc886a8d3fb8a5cfd`. Consulted applicable pinned AGENTS.md, current private KnowledgeBase v1.5.8, PROJECT_STATUS, WORKFLOW, EVIDENCE_INDEX and RECORDKEEPING_STANDARD. Current R20-centric documentation and its R21/R22 synchronization gap remain unchanged. No private KnowledgeBase contents are published here.
- Reused [TEST02 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1559b557db69abd4ef6d152fc886a8d3fb8a5cfd/docs/codex-berichte/2026-11-14_r22-voice-copy-encoder-navigation-test02.md), independently fetched and verified Git blob `7920c1fa693284bcf496af203ade35faa114db96`. Rechecked CI `43647784692` and `43639972875`: both remain successful on their exact TEST02/TEST01 source pins. Their original artifacts and reports were not modified.
- All 130 existing remote heads were identical before implementation and after stopping, including main, lab-setup, release/r22, release/r21, R4/R11/R18/R19/R20/CLP and historical report references. The only publication from this task is this new report branch; no protected head is advanced.
- No main merge, force-push, release, hardware test, installation, flashing, audio change, bootloader change, branch reorganization or project-wide documentation consolidation. Current usage allowance/reset is UNKNOWN. The task was kept to the selector, inherited tests and one planned ARM build; the build was not reached.

## Confirmed source defect and retained draft

The defect is settled by exact source; no repeated investigation was needed. At `Core/Src/menu/voiceTransfer.inc:176`, TEST02 formats the internal destination plus one. The transaction passes the unmodified index to `preset_transferVoiceTo`. Normal LOAD/SAVE formats the raw slot; `preset_makeFileNameSave` at `Core/Src/Preset/presetManager.c:2533` passes the raw index to the two-digit filename formatter. The actual RAM/storage index already agrees with 00–63.

The only production edit in the uncommitted draft is:

```diff
-        uint16_t kit = (uint16_t)menu_voiceCopyTarget + 1;
+        uint16_t kit = (uint16_t)menu_voiceCopyTarget;
```

The whole production-tree diff contains no other Core/Drivers/Middlewares changes. Preset transfer/mapping/LFO/Morph/storage, normal LOAD/SAVE, sound/FX/CLP, headers, parameter fields, menu dispatcher, panel code and main/boot remain byte-identical to TEST02. The boot string remains `LXR LAB R22`. No index migration or new buffer is introduced.

The retained six-file draft (+94/−23 lines) also:

1. Amends existing LCD expectations in `tests/r21_voice_copy/host.c`, `tests/r22_drum_copy/cases.c` and `tests/r22_drum_copy/navigation_cases.c`, retaining exact 16-cell/guard/cursor assertions. The former slot-1 K02 expectation becomes K01; slot-9 K10 becomes K09; endpoints become K00/K63.
2. Adds a literal independent K00–K63 oracle to the existing navigation fixture, without production-format arithmetic. It intends to cover all seven source pages ×64 slots, exact RAM-byte isolation, eight real FatFs project/file mappings for slots 00/01/09/63 on Drum and non-Drum sources, and seven explicit same-kit rejections. Target 9 uses source identity 8 rather than shifting the destination to evade same-kit rejection. **The full planned matrix did not complete.**
3. Changes the inherited `r22-drum-copy-validation.yml` candidate/artifact labels and `validate_arm.py` output names to TEST03, with matched TEST02 resource constants. This workflow draft was not committed or run.

The local source checkout stays at the pinned TEST02 commit with these six files modified. Do not accidentally publish the draft as validated. The exact diff and complete added oracle are preserved in the report's text evidence and can be reconstructed from the pinned base.

## Required-test failure and decisive explanation

Command: `python3 tests/r22_drum_copy/run.py`, called by the retained regression driver. The suite compiles the production selector/transfer/public panel/encoder paths and real FatFs fixture under ASan/UBSan. Compilation completed. The process then aborted with SIGABRT; wrapper exit code was **1**:

```text
host: /workspace/lxr-desktop-app/tests/r22_drum_copy/navigation_cases.c:253: slot_label_oracle: Assertion `!memcmp(activeParameters,expected,sizeof expected)' failed.
```

The first new oracle iteration is source Drum 1, destination internal index 0. Before reaching this assertion, the new case successfully checked K00 in Kit navigation, bracketed editing and OK, exact destination RAM index 0 and all other kit bytes, active/engine/pattern preservation and zero transfer-time SD/FX/front-panel dispatch. It then checked `/proj00/00-KITNA.snd` against the exact expected destination bytes and reloaded the project into RAM with identical destination bytes. The loaded kit identity also equalled 0. The failure is the subsequent whole-active-buffer comparison after `preset_loadDrumsetRam(0,0,1)`.

**Source-established fixture conflict:** `tests/project_transaction/whole.c:170` zeros all RAM kits. Existing `reset_r21` initializes valid LFO voice IDs only in active parameters and destination kit 1, not kit 0. The new oracle therefore expects zero `PAR_VOICE_LFO1..6` fields for kit 0. The unchanged production immediate-load function, `presetManager.c:1191–1193`, explicitly converts any LFO voice ID outside 1–6 to 1:

```c
if(activeParameters[PAR_VOICE_LFO1+i] < 1 || activeParameters[PAR_VOICE_LFO1+i] > 6 )
    activeParameters[PAR_VOICE_LFO1+i]=1;
```

Thus the new assertion is too strict for this deliberately invalid fixture after a normal load. It must not be presented as a discovered kit-index/storage corruption or a reason to alter the loader. The original source and the existing KIT1-focused roundtrips remain unchanged. The failed run did not print a per-parameter byte-difference trace; the attribution above follows the pinned seed, exact assertion location and unchanged load path. Buffered R22 PASS messages were not flushed before abort; the full raw stderr/traceback is retained.

**Proposed correction, NOT IMPLEMENTED OR RETESTED:** initialize valid LFO voice IDs for the selected destination in the new oracle's setup before taking its snapshot. Keep the full RAM/file/reload assertions and the production load normalization unchanged. Then rerun the complete oracle and required regression/ARM gates. Do not remove the assertion, alter preset semantics or shift a target by one. The new cases for the other 63 slots, remaining source pages, all eight planned project roundtrips and the final seven same-kit checks cannot be claimed passed from this aborted run.

The attached task explicitly states: **“If a required test fails, ... STOP, explain the blocker and do not produce or publish a misleading passing candidate.”** Execution stopped at that gate; there was no correction/rerun after observing this failure and no ARM build. A fixture-only continuation requires an explicit owner resume decision under this stop instruction.

## Commands and actual results

Host compiler `cc (Debian 14.2.0-19) 14.2.0`, Python 3.12.14 and exact arguments are retained in the run ledger and driver. The existing driver scheduled independent suites with three workers; those suites and two standalone checks were already completed when their collected results exposed the R22 failure. No further firmware test was started after that observation.

| Executed check | Result |
| --- | --- |
| `python3 tests/r22_drum_copy/run.py` | **FAIL**, exit 1 / host SIGABRT at new oracle line 253 |
| `python3 tests/r21_voice_copy/run.py` | PASS, exit 0; retained Morph 57→123, identity, same-kit/context, six engines/shared HH, LFO, persistence and panel assertions |
| `python3 tests/project_storage/run.py` | PASS, exit 0 |
| `python3 tests/project_transaction/run.py` | PASS, exit 0 |
| `python3 tests/r20_ui/run.py` | PASS, exit 0 |
| `python3 tests/r20_ui/integration/probe.py . /workspace/r22-test03/r20-integration` | PASS, exit 0 |
| Existing Delay / USB-MIDI / Compressor host runners | Each PASS, exit 0; exact argv in `regression-results.json` |
| `python3 tests/fx_clip_r18/compare.py` | PASS, exit 0 |
| `python3 tests/bug001/run.py` | PASS, exit 0 |
| BUG008 capture C11/UBSan build/run | PASS, compile/run exit 0 |
| FIX8 playback C11/UBSan build/run | PASS, all 16,384 value pairs / 106,792 changes; compile/run exit 0 |
| `git diff --check` on source draft | PASS |
| New ARM GNU14.3.Rel1 Release CI | **NOT RUN** because required R22 gate failed |
| TEST03 IMG/ZIP/updater/ELF/MAP/resource comparison | **NOT PRODUCED / NOT RUN** |

The R22 runner executes its existing nine mappings, 306 LFO targets, same-kit/Morph/persistence and TEST02 navigation cases before reaching the appended oracle. Reaching the new failure is consistent with those preceding assertions completing, but the aggregate R22 result is failed and is not promoted to PASS. No inherited assertion was removed or weakened. No repeated candidate build or historical reproduction was run.

## Build, resource and retention status

There is **no new TEST03 source SHA, CI run/job/artifact ID, IMG/ZIP checksum, ELF/MAP hash or static resource delta**. Expected candidate filenames were drafted as `LXR_V2_R22_TEST03.img` and one-image `LXR_V2_R22_TEST03.zip`; neither was produced or delivered. Updater, boot, segment/ZIP identity and resource checks remain pending. Do not rename TEST02 bytes and call them TEST03.

Inherited TEST02 comparator only: ARM GNU14.3.Rel1 Release, CI 43647784692, attempt 1/job 132609298661, artifact 14134945492, expires 2026-12-14T22:23:43Z. `.text` 160,728 B; `.rodata` 83,272 B; flash payload 297,680 B; ITCM 15,304 B; DTCM span 107,424 B; SRAM span 275,164 B. Original IMG 297,696 B / SHA256 `f3a39aa792cc7b54ae9e260bdc6bedd45dbbb28934da43035228b26f7e58c9f3`; ZIP 213,653 B / SHA256 `9f85a4b4fdb9b037837b5291b105894b87ce69fa8862cf28753c301846b0c82c`. These are reused evidence, not new TEST03 results. TEST01 CI 43639972875 also remains preserved.

CPU/WCET, DMA/IRQ deadlines, runtime stack high-water, long-run reliability and real-device audio are **UNMEASURED**. No new hardware result exists. Permanent binary archive: **NOT ARCHIVED**; no TEST03 binary exists, and no new permanent destination was verified for prior binaries. Existing Actions artifact expiry remains applicable.

## Evidence and next owner decision

[Reproducible text evidence](../evidence/r22-kit-index-display-test03-stop/README.md) includes exact failing draft, independent oracle, full raw failure/other suite logs, machine-readable results/commands, original CI identity checks, source excerpts, boundary hashes, protected-ref verification and SHA256 manifest. No private KnowledgeBase, credentials or firmware binaries are committed. Older TEST01/TEST02 evidence was not modified. The new report branch/file is independently fetched and verified after push; it is not a firmware source branch.

**NEXT OWNER DECISION:** authorize a fixture-only correction for valid destination LFO voice IDs and resume the same TEST03 validation, retaining this failure and the exact one-line production scope. Final R22 stable acceptance remains a separate decision. Broader KnowledgeBase/workflow/documentation consolidation remains deferred to the owner's separate conversation.

For a separately authorized future device session, NOT RUN here: verify K00/K01/K09/K63 correspond to the same-numbered LOAD/SAVE kits and remain so after SAVE + PROJECT/reload; confirm no shifted kit, one cross-Drum copy, one same-kit rejection, VOICE cancel without STEP and COPY ignored in the dialog. This checklist does not authorize device installation or testing.
