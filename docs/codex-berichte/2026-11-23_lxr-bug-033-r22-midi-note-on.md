# LXR-BUG-033 — R22 duplicate MIDI Note On: isolated fix candidate

**Date:** 2026-11-23 (Europe/Vienna). **Result:** exact-source host regression PASS and ARM Release PASS; **owner device test NOT RUN, no release/stable approval**.

## Source, authority and scope

Owner approval in ChatGPT: "freigabe erteilt" after the explicit plan for a vendor-derived, isolated R22 bug fix, focused tests, ARM build and resource comparison, with no physical device test or main merge.

- Source repo: DocLXR-02/lxr-desktop-app.
- Accepted R22 TEST03 source base: 0e37942250f79306627702061d851731bdfdbbc8 (protected unchanged).
- Isolated fix branch: fix/lxr-bug-033-r22-note-on.
- **Actual validated firmware candidate SHA: 4986342411f1776962e0c74a9dd0796d54e6c25b.**
- Lab authority read: DocLXR-02/lxr-firmware-lab SHA c8ffbcee4e8036f3e551570886f46730867d6bef. Source AGENTS.md read at accepted R22. R22 is 66 commits ahead of the lab-setup tip 5829398cea5d337200393757e39a37d4d8148256, reconciling its ancestry requirement.
- This handoff is on an independent report-only branch based on lab-setup, NOT in the tested source commit.
- Excluded: unrelated DSP/FX/SD/preset/clock changes, physical device operation, firmware release, merger of any branch into main, accepted R22 or R18 reference changes.

## Evidence of the existing defect

Original R22 seq_triggerVoice() calls voiceControl_noteOn(), which sends a MIDI Note On with the step note; seq_triggerVoice() then sends a second Note On using the voice override. Only the last note for each MIDI channel is remembered for Note Off. With step note 63 and override 48, the old code sends two notes (63 and 48) but only turns off note 48.

Manufacturer merged PR #12 on 2026-11-23: https://github.com/erica-synths/lxr-desktop-app/pull/12
Merge commit: https://github.com/erica-synths/lxr-desktop-app/commit/5563346c7fb6ff3fda9390ecd126a37df624fb73

The upstream author's 12-hit A/B hardware test showed the fault and the targeted correction for one voice; other voices and USB were not physically tested. The separate R23 Lab approach removes MIDI sends from voiceControl_noteOn() entirely, altering direct input behavior, so it was NOT used for this R22-only candidate.

## Firmware change: exactly four source files

- Core/Inc/Soundengine/MIDI/MidiVoiceControl.h — voiceControl_noteOn() gains a sendMidi parameter.
- Core/Src/Soundengine/MIDI/MidiVoiceControl.c — sendMidi gates the extra MIDI send; internal audio trigger, front-panel LED and note-off machinery remain.
- Core/Src/Soundengine/MIDI/MidiParser.c — direct MIDI input passes sendMidi=1 to preserve original input echo.
- Core/Src/Soundengine/Sequencer/sequencer.c — internal sequencer passes sendMidi=0 and retains its own override-aware note transmission.

Exact source compare:
https://github.com/DocLXR-02/lxr-desktop-app/compare/0e37942250f79306627702061d851731bdfdbbc8...4986342411f1776962e0c74a9dd0796d54e6c25b

## Tests actually executed

1. Focused production-source host oracle: python3 tests/lxr_bug_033/run.py. Extracts the production C function bodies from both baseline R22 and fixed candidate; independent event assertions observe seven voices, override vs any note, Note Off, repeated step, Stop, MIDI input direct echo, transmit filter, invalid voice, distinct MIDI channels and matching logical DIN/USB sends. **25 cases per source per sanitizer, four executions (R22 and candidate with UBSan and ASan): PASS**. The original source reproduces duplicate Note On and absent Note Off; the candidate emits one sequencer Note On and the correct Note Off. This host fixture does NOT simulate actual cables, IRQ scheduling or the full MIDI parser.
2. Inherited R22 voice-copy/selector/project validation: python3 tests/r22_drum_copy/run.py — PASS.
3. Inherited USB-MIDI transport host regression: python3 tests/r18_usb_midi/run.py build/lxr-bug-033-usb — PASS.
4. ARM GNU Toolchain 14.3.rel1 (sha256-checked download): cmake --preset release ; cmake --build --preset release --parallel 2 — PASS.
5. Firmware ELF/updater image checks, header checksum, boot string LXR LAB R22, program segments and static memory bounds: python3 tests/lxr_bug_033/validate_arm.py — PASS.

Successful [CI run 45244904035](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45244904035), source SHA 4986342411f1776962e0c74a9dd0796d54e6c25b.

**Preserved historical STOP:** [CI run 45244789109](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45244789109) failed before C compilation because the new test extractor did not match a C function declaration whose opening brace is on the next line. Correcting the regex in the test-only revision led to the full successful CI. The original fail is not overwritten and is NOT evidence of a faulty firmware candidate.

## Static resource comparison (bytes), matched with accepted R22 TEST03 report

| Region | R22 TEST03 | This candidate | Delta | Remaining capacity |
|---|---:|---:|---:|---:|
| .text | 160712 | 160704 | -8 | — |
| .rodata | 83272 | 83272 | 0 | — |
| ITCM | 15304 | 15304 | 0 | 1080 |
| DTCM | 107424 | 107424 | 0 | 23648 |
| SRAM | 275164 | 275164 | 0 | 118052 |
| Flash payload | 297664 | 297656 | -8 | 718152 |

All declared regions within static limits. **CPU/WCET, IRQ/DMA deadline margins, runtime stack high-water and long-term reliability UNMEASURED.** Not a real-time certification.

**Exact candidate output:**

- IMG LXR_BUG_033_R22_CANDIDATE_NOT_FOR_FLASHING.img: 297672 B; SHA-256 98a725a47ba512713cdc91552c0a27e31c40846dd6b9c21bed29e8530b783b90.
- ELF LXR_V2_STM32F765.elf: 404496 B; SHA-256 825f39ae365f853eed96e6deb91040e67c410f84185edb70b7443133f6f59f83.
- MAP LXR_V2_STM32F765.map: 552838 B; SHA-256 392074a7539a22f7315f69b85ab163985c3d86fe816c14a82b7b5a4198af4766.

## Original evidence retained and verified

- [GitHub CI source artifact 14854380415](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45244904035/artifacts/14854380415), original ZIP 617258 B, SHA-256 5af37f7eca5e8a0c1fd298e617382eca675bc2155b271c8d146c0521b595c64b. Actions retention until 2026-12-23.
- The [Lab automated archive run 45245144492](https://github.com/DocLXR-02/lxr-firmware-lab/actions/runs/45245144492) recorded SOURCE ORIGINAL VERIFIED, committed the exact original ZIP on archive-only branch [commit b77084f](https://github.com/DocLXR-02/lxr-firmware-lab/commit/b77084f), then verified original bytes/indexes from the remote branch as REMOTE ORIGINAL PASS.
- [Archived original ZIP](https://github.com/DocLXR-02/lxr-firmware-lab/blob/b77084f/archive/objects/evidence/CI/runs/45244904035/artifact-14854380415-lxr-bug-033-r22-evidence-4986342411f1776962e0c74a9dd0796d54e6c25b.zip)
- [Run provenance JSON](https://github.com/DocLXR-02/lxr-firmware-lab/blob/b77084f/archive/provenance/CI/runs/45244904035.json).

## Remaining risks and next decision

The manufacturer's fix and this isolated candidate solve the duplicated **sequencer** Note On path. The original MIDI parser has a separate record/echo path that can send again when do_rec is enabled; not changed or full-path tested. Multiple voices on one MIDI channel still share only one remembered Note-Off target. These are different, explicitly out-of-scope questions.

**No physical LXR-02 test, MIDI cable/USB device capture, firmware flashing, device audition, owner hardware acceptance or stable release has occurred.** Do not install this validation image on the instrument without a separately approved recovery/backup and hardware-test protocol. Keep accepted R22 TEST03 as the recovery reference.

The branch-scoped CI workflow and test scripts are retained as a *reusable, explicitly bounded validation* for the candidate, not an unattended disposable test PR. No cleanup of source, branches, historical evidence or workflow is authorized or performed; no temporary test-only PR was opened.

**Next gate:** Device-return and independent MIDI DIN/USB monitoring, once separately approved; or an additional narrow host test if the owner first wants broader input echo coverage. No merge or release follows automatically.
