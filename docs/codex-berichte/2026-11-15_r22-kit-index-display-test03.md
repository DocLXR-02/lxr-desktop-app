# R22 TEST03 — Correct kit-index display: resumed build report

Date: 2026-11-15 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**Result: PASS for required local host gates, one exact-source ARM Release CI build and independent candidate verification. TEST03 IMG/ZIP truly exist and are delivered. This is a test candidate; no TEST03 hardware/audio test, installation, flashing, merge, stable acceptance or public release occurred.**

## Identity, authorization and reused evidence

- Source branch **`release/r22`**, exact verified TEST02 base / direct parent **`4372e60337bf5ce35d67e8b5eb577baa3fffb962`**.
- Single additive TEST03 source commit **[`0e37942250f79306627702061d851731bdfdbbc8`](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8)**. No rebase, merge, force-push, tag, deletion or source history rewrite.
- New report-only branch **`reports/r22-test03-2026-11-15`**, rooted in unchanged `lab-setup` **`5829398cea5d337200393757e39a37d4d8148256`**. Firmware was built from the source branch, not this documentation branch.
- Accepted TEST01 ancestor `ace7ad95081aac6a015f11b87b6c540ce100b344`, [CI 43639972875](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43639972875), remains protected.
- Reused [immutable TEST02 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1559b557db69abd4ef6d152fc886a8d3fb8a5cfd/docs/codex-berichte/2026-11-14_r22-voice-copy-encoder-navigation-test02.md), [CI 43647784692](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43647784692), artifact 14134945492, and original verified TEST02 IMG/ZIP/ELF/MAP for matched comparison. No historical build was repeated or prior artifact renamed to create TEST03.
- Reused [immutable TEST03 stop report](https://github.com/DocLXR-02/lxr-desktop-app/blob/bf9162ae0b131c97a12d3ea4ab472a45d96f4cc8/docs/codex-berichte/2026-11-15_r22-kit-index-display-test03-stop.md) and its complete draft/failure evidence. The owner explicitly authorized only the diagnosed fixture precondition correction and resumption on 2026-11-15. The historic failure remains FAIL.
- Preflight verified live `release/r22` exactly at TEST02, applicable AGENTS.md at that SHA, the local six-file draft byte-for-byte against published `draft.patch` (Git blob `4da73f111718bacf913f52cb9690a9dd1e19f96f`), and unchanged stop/guidance refs. KnowledgeBase v1.5.8 and canonical PROJECT_STATUS/WORKFLOW/EVIDENCE_INDEX/RECORDKEEPING_STANDARD were reused from the current session and checked at unchanged `docs/lab-project-state` **`9b68be76d80de50cb84fa9882c5b9003bcb6faf8`**. No private KnowledgeBase content was published.
- The existing public snapshots remain R20-centric. Project-wide KnowledgeBase/GitHub/platform consolidation is deferred to the owner's separate conversation; the R21/R22 indexing gap is not represented as closed. Allowance/reset remains UNKNOWN. This bounded resume reused diagnosed evidence and existing runners; no exploratory re-audit or subsequent task was started.

## Defect, sole production correction and fixture history

OWNER HARDWARE feedback: TEST02 cross-Drum copying works, but copy labels are one greater than the normal zero-based LOAD/SAVE kit number. This is bounded owner feedback, not a full device coverage matrix or TEST03 stability approval.

SOURCE finding, already established by the prior task: `menu_renderVoiceTransfer` in `Core/Src/menu/voiceTransfer.inc:176` added one only for the label. Existing `preset_transferVoiceTo` receives the actual target index unchanged; `.snd` serialization and normal LOAD/SAVE use indices 00–63. No kit data migration is needed.

The **entire production change** is:

```diff
-        uint16_t kit = (uint16_t)menu_voiceCopyTarget + 1;
+        uint16_t kit = (uint16_t)menu_voiceCopyTarget;
```

The renderer still fills exactly 16 cells with the same brackets, cursor, immutable source/Drum labels and `>OK`. It now renders the actual selected index directly:

| Internal slot | Copy label | Actual file prefix |
| ---: | --- | --- |
| 0 | K00 | `00-` |
| 1 | K01 | `01-` |
| 9 | K09 | `09-` |
| 63 | K63 | `63-` |

The retained new oracle initially failed after loading slot 0: the base fixture seeded invalid zero `PAR_VOICE_LFO1..6` IDs in that destination. The unchanged loader correctly normalizes invalid voice IDs to 1; an equality assertion for the complete loaded sound-parameter record therefore failed. The earlier RAM, `.snd` bytes and project reload comparisons had already passed. That failure is retained as `HISTORIC_STOP_r22.log`, with the original stop ledger/result manifests; it was not rerun, deleted, rewritten or relabeled PASS.

The owner-authorized **fixture-only** addition in `slot_label_oracle`, before its pre-transfer snapshot and expected array, is exactly:

```c
/* Valid preset precondition: immediate load must not normalize zero IDs. */
for(unsigned lfo=0;lfo<6;lfo++)mainParameters[kit][PAR_VOICE_LFO1+lfo]=lfo+1;
```

Each selected destination has deterministic valid voice IDs 1–6. No assertion is weakened or removed; exact RAM/file/project/loaded-sound-parameter equality and all inherited invariants remain. The production loader, LFO/target mapping and transfer behavior were not edited. `authorized-fixture-fix.patch` records the two added lines against the immutable failed draft.

Final six-path diff (+96/−23 lines), exactly within authorization:

| Path | Change |
| --- | --- |
| `Core/Src/menu/voiceTransfer.inc` | The one-line presentation correction only |
| `tests/r21_voice_copy/host.c` | Slot 1 display expectation K02→K01 only |
| `tests/r22_drum_copy/cases.c` | Two corresponding LCD expectation updates only |
| `tests/r22_drum_copy/navigation_cases.c` | Correct zero-based expectations; retained independent 64-slot oracle; authorized valid-LFO setup |
| `tests/r22_drum_copy/validate_arm.py` | TEST03 output/labels and actual matched TEST02 resource baseline |
| `.github/workflows/r22-drum-copy-validation.yml` | TEST03 display/output/artifact naming; all gates retained |

Full production-tree comparison proves every other Core/Drivers/Middlewares file byte-identical to TEST02. Preset transactions, numeric indices, fields, storage formats/layout, save/load, Morph guard/loaded-kit identity, DSP/LFO/FX/CLP/audio, UI input state machine, dispatcher, boot/main and bootloader are untouched. Boot remains **`LXR LAB R22`**. No new buffer, control, parameter or same-kit feature.

## Executed local tests and exact-source CI gates

The focused R22 command ran **first**, before committing source: `python3 tests/r22_drum_copy/run.py`, exit 0, complete PASS. Then the established three-worker driver ran 10 inherited regressions, each exit 0. The focused suite was not needlessly repeated locally by that driver; CI subsequently ran it again on the exact new source SHA. Local compiler: `cc (Debian 14.2.0-19) 14.2.0`; Python 3.12.14. The existing R21/FatFs fixture retains ASan/UBSan, non-recovering sanitizer checks and `ASAN_OPTIONS=detect_leaks=0`. Hardware/audio boundaries are adapters; this is not a whole-device emulator.

| Check | Actual result |
| --- | --- |
| Literal independent K00–K63 oracle across all 7 source pages × 64 target slots | PASS, all 448 cases |
| Navigation/edit/OK LCD rows, exact16 cells, cursor bounds and adjacent guards | PASS |
| Correct RAM destination and every non-target kit byte, source/engine/pattern preservation, zero transfer-time FX/front-panel/SD dispatch | PASS |
| Real FatFs project SAVE/reload, `.snd` prefix/content and immediate kit-load equality for 00/01/09/63, Drum 1 and non-Drum V5 | PASS, all 8 roundtrips |
| Explicit added same-kit rejection cases | PASS, all 7; no side effects |
| Inherited all 9 cross-kit Drum mappings, independent 25-field oracle, all 306 eligible LFO translations/override/exact conflicts | PASS |
| Inherited same-kit/context/transport rejection, Morph 57→123 and zero/nonzero/pending/unrelated/commit guard, shared HH, unsaved source, STEP hold/release, project persistence | PASS |
| DATA Kit/Drum/OK navigation, VOICE cancel without STEP, ignored COPY and normal outside controls | PASS |
| R21 voice-copy suite, project storage, project transactions, R20 UI and full integration | PASS locally and in CI |
| Delay, USB-MIDI, Compressor, R18 CLP compare, BUG001 parameter sweeps | PASS locally and in CI |
| BUG008 capture, exhaustive FIX8 playback and adaptive-removal source checks | PASS on new-SHA CI; prior local successes reused, not rerun solely for duplicate logs |
| Source whitespace and exact six-path/one-production-line scope checks | PASS |

The oracle uses 64 literal labels, not the production formatter/arithmetic. It navigates to the requested exact slot; it does not offset the selector to make a label pass. For destination 9, it deliberately captures from source kit 8 so that K09 can be tested as a permitted cross-kit target. Same-kit rejection is tested independently and remains intact. All normal non-Drum/shared-HH behavior is retained. Detailed commands, flags, exit statuses and raw outputs are in `run-ledger.json`, `regression-results.json`, the pinned source runners and CI logs.

No new local, behavioral, sanitizer or CI failure occurred during this authorized resume. The prior stopped failure is recorded separately. The source commit is the exact tested draft; its sole parent is TEST02. One source push triggered **exactly one candidate ARM build**; no rerun/repair or local ARM build was needed.

## ARM build and original artifact

- [CI 43707621969](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969), attempt 1, [job 132792019014](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969/job/132792019014): **SUCCESS** at exact source `0e37942250f79306627702061d851731bdfdbbc8`. All host/source/build/validator/upload steps completed successfully.
- Existing `.github/workflows/r22-drum-copy-validation.yml`, Ubuntu 24.04, `cmake --preset release`, `cmake --build --preset release --parallel 2`. GitHub retains the prior workflow display name in run metadata; checked-out TEST03 workflow and artifact identity are verified at the exact commit.
- Compiler: `arm-none-eabi-gcc (Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174)) 14.3.1 20250623`. Toolchain archive SHA256 `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd` checked before use. Full compiler/configuration/build/section logs retained.
- [Artifact 14159486119](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969/artifacts/14159486119), `r22-test03-kit-index-build-evidence`: original CI archive **756,063 B**, SHA256 **`aa0d442a662d1720647ccae9b73f4d4367fc20e4eb0523ef385a6cab09692c69`**; created 2026-11-15T11:11:11Z, expiry **2026-12-15T11:11:10Z**, retention 30 days. Original downloaded archive CRC/digest verified; it was not repacked.
- Original archive contains IMG, one-image ZIP, ELF, MAP, source/compiler/section manifests, validator results and all workflow logs. Local separate copy: `/workspace/deliverables/evidence/LXR_R22_TEST03_CI_Evidence_43707621969.zip`.
- **Permanent binary archive: NOT ARCHIVED.** Actions retention is finite; workspace paths are transient, not verified permanent storage. Public Git retains text evidence only, no firmware binaries or private KnowledgeBase.

| Original CI file | Bytes | SHA256 |
| --- | ---: | --- |
| `LXR_V2_R22_TEST03.img` | 297680 | `da1c882f6600c96fa5926ad4797d618ce13eefc16ab5563b8a161686f1909c0b` |
| `LXR_V2_R22_TEST03.zip` | 213625 | `1426b2e4fab334ef852b0fda053a035e862e71b05bcbd496a277e2863d901466` |
| `LXR_V2_STM32F765.elf` | 404496 | `3ff7ad96709a3191d8680e3b28122b0d313564b415391c3a20e0aa2588431cf1` |
| `LXR_V2_STM32F765.map` | 552877 | `1acc16f08db112dde38022c575ce5805317a808db25b7fa02fccf1f224093021` |

## Independent binary verification and matched resources

`verify-delivery.py` checked the original CI archive digest/CRC, exact source, all four output byte counts/hashes, updater header, ELF load segments and single-image ZIP identity independently after download. IMG header magic `LXRV2IMG`; payload 297,664 bytes; checksum byte 45 (`0x2d`, `255 - (sum(payload) & 255)`); reserved bytes 0. Boot bytes `4c5852204c41422052323200` at IMG offset 161,536 are the unchanged NUL-terminated R22 label. CI also checks vector SP alignment/range and Thumb reset entry. Every file-backed ELF PT_LOAD segment equals the IMG payload at its physical load address, and the maximum load extent equals the full payload.

`LXR_V2_R22_TEST03.zip` contains exactly **one** `LXR_V2_R22_TEST03.img`, original internal filename retained, correct CRC and byte-identical to the standalone IMG. Delivered files `/workspace/deliverables/LXR_V2_R22_TEST03.img` and `/workspace/deliverables/LXR_V2_R22_TEST03.zip` are unmodified original CI copies. TEST02 bytes were not renamed or substituted.

The matched comparator is the actual original TEST02 ARM GNU14.3.Rel1 CMake Release artifact; its file hashes and source were reverified. These are region spans including alignment/reservations, not CPU estimates:

| Bytes | TEST02 | TEST03 | Delta | TEST03 static headroom |
| --- | ---: | ---: | ---: | ---: |
| .text | 160728 | 160712 | -16 | — |
| .rodata | 83272 | 83272 | 0 | — |
| ITCM | 15304 | 15304 | 0 | 1080 |
| DTCM | 107424 | 107424 | 0 | 23648 |
| SRAM | 275164 | 275164 | 0 | 118052 |
| FLASH_payload | 297680 | 297664 | -16 | 718144 |

Complete IMG including 16-byte updater header: 297,696→297,680 (−16 B). No RAM growth: `.data` 852 B, `.bss` 273,796 B and reserved heap 512 B remain unchanged. DTCM headroom 23,648 B is before runtime stack; nominal ITCM headroom 1,080 B does not describe CPU reserve.

The reused asserted Thumb-prologue decoder compares both actual ELF/IMG pairs. Fixed entry frames (saved registers plus immediate SP allocation): `preset_transferVoiceTo` 320→320 B, `menu_parseEncoder` 40→40 B, `menu_voiceTransferButton` 24→24 B, `menu_repaint` 88→88 B. All deltas 0. The selector encoder/renderer are inlined in the latter menu functions. Exact prologue bytes/addresses/masks and ELF hashes are retained. These checks exclude later path allocations, callers/callees and interrupt frames; they are not a whole-call-tree bound or measured stack high-water.

**UNMEASURED:** CPU/WCET, IRQ/DMA deadlines, stack high-water, long-run reliability and actual device audio/acoustics. No physical TEST03 installation/panel/display test was performed. Host/CI/static fit do not certify those properties or declare R22 stable. R18 accepted CLP sound and R20 stable firmware remain protected historical references.

## Preservation, evidence and next owner decision

Before source push every remote head still matched the preflight snapshot. After source push, only authorized `release/r22` advanced from TEST02 to TEST03; **130 other existing heads remained unchanged**, including immutable TEST03 stop, TEST01/TEST02 reports, main, lab-setup, R4/R11/R18/R19/R20/R21/CLP and docs. Report publication adds only this separate documentation branch. The published selector was independently fetched at its exact source URL and matched local Git blob `d197f2da8569870a6da5067bb5921114c7cd3a16`. Report/file content and final remote heads are independently verified after push.

[Reproducible evidence](../evidence/r22-kit-index-display-test03/README.md) retains the historic raw FAIL, corrected PASS, exact fixture fix and source diff, commands/manifests, all original CI text outputs, binary verification and matched stack evidence. Stop report/source history/artifacts were not overwritten. Source branch and report branch remain distinct; no deferred consolidation was started.

**NEXT OWNER DECISION:** review the verified TEST03 candidate and decide separately whether to authorize installation/device testing and ultimately close R22 as stable. No installation, hardware test or stable release acceptance is granted by this report.

Proposed short device checklist, **NOT RUN** here and subject to separate authorization: verify K00/K01/K09/K63 refer to the same-numbered LOAD/SAVE kits before and after SAVE + PROJECT/reload; confirm no shifted kit; check one cross-Drum copy, one same-kit rejection, VOICE cancel without STEP and ignored COPY inside the dialog. No subsequent task starts automatically.
