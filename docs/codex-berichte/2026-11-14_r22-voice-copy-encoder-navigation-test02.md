# R22 TEST02 — Voice-copy encoder navigation candidate

Date: 2026-11-14 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**Result: host/source gates and one matched ARM Release CI build PASS. TEST02 is a test candidate, not stable or a public firmware release. No hardware test, installation, flash, merge or acoustic acceptance was performed or authorized.**

## Identity, authorization and protected boundaries

- Source branch: `release/r22`; exact required and verified starting head / direct parent: `ace7ad95081aac6a015f11b87b6c540ce100b344` (R22 TEST01).
- Additive TEST02 source commit: **`4372e60337bf5ce35d67e8b5eb577baa3fffb962`**. No force-push, merge or history rewrite. TEST01 remains its parent.
- Separate report-only branch: `reports/r22-test02-2026-11-14`, rooted in unchanged `lab-setup` **`5829398cea5d337200393757e39a37d4d8148256`**. This report branch contains documentation/evidence only.
- Source scope: DATA navigation, per-field editing, explicit OK, physical VOICE cancellation and safe consumed input edges. The owner's TEST01 device observation concerned the UI: DATA push confirmed immediately and instrument keys selected target Drum. It is not a full physical coverage matrix.
- Owner-approved Alternative C remains cross-kit only, all nine Drum1–3 mappings, existing R21 inactive-target RAM architecture. All same-kit transfers remain rejected. R21 Morph guard and loaded-kit identity correction remain unchanged.
- Only production file changed: `Core/Src/menu/voiceTransfer.inc`. The complete production-tree diff proves no other Core/Drivers/Middlewares changes. `menu.c`, `buttonHandler.c`, `copyClearTools.c`, preset transaction/mapping/Morph/storage code, headers, DSP/LFO/FX, main/boot and startup remain byte-identical to TEST01. Hashes of central boundaries are retained in evidence. No new RAM buffer.
- Live remote checks before/after source push: the only changed existing head was authorized `release/r22`; **128 other existing heads were unchanged**, including main, lab-setup, docs, R4/R11/R18/R19/R20/R21/CLP and previous R22 reports. Protected refs and TEST01 artifacts were not overwritten.
- `docs/lab-project-state` remains `9b68be76d80de50cb84fa9882c5b9003bcb6faf8`. The private KnowledgeBase v1.5.8 and public R20-centric snapshots were consulted as context; the open R21/R22 documentation synchronization gap remains. Private KnowledgeBase contents are not published here. Allowance/reset is UNKNOWN; no estimate was made.

## Reused evidence and decisive source findings

[R22 TEST01 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/reports/r22-test01-2026-11-14/docs/codex-berichte/2026-11-14_r22-drum-cross-voice-copy-test01.md), immutable report commit `59628c8e0591d70e1812aeda6f6455aee19688b7`, supplies the accepted mappings, source-connected storage/LFO/Morph tests and matched ARM baseline. Its CI run `43639972875`, attempt 1/job `132585174084`, artifact `14130863884`, was reused for binary/resource comparison. The original TEST01 IMG, ZIP, ELF, MAP and archive remain intact.

The earlier [same-kit stop report](https://github.com/DocLXR-02/lxr-desktop-app/tree/6ba37b023857a02b63ace0f1faa2fde24b4dd33a/docs) and R21 reproductions were retained and not repeated as a new research exercise. Same-kit live modulation synchronization remains intentionally unsupported. The original R21 57→123 Morph regression remains in the inherited R21 suite and passes below. Owner-reported R21 device success does not certify R22 or TEST02.

At TEST01, the selector treated a ready DATA click as immediate transfer and intercepted Drum instrument keys as target selection. Both existing integration hooks already precede normal dispatch: `menu_parseEncoder` in `menu.c:2920`, and public pressed/released panel dispatchers in `buttonHandler.c`. The normal private VOICE handler can toggle an existing Voice page into STEP. The existing `buttonHandler_setMode` only sets UI mode; it does not run that normal toggling dispatcher. `menu_switchPage` can issue a sequencer query, so cancellation restores the saved source page directly and updates UI mode/LED/cursor state without calling it. `menu_resetActiveParameter` changes only menu cursor bits.

## Implementation and panel contract

[Exact selector source](https://github.com/DocLXR-02/lxr-desktop-app/blob/4372e60337bf5ce35d67e8b5eb577baa3fffb962/Core/Src/menu/voiceTransfer.inc): local focus is Kit/Drum/OK, with an independent local editing flag. Navigation and value turns are bounded, one step per signed detent. Non-Drum sources skip Drum. Initial kit remains source kit, initial Drum is source instrument, initial focus Kit without editing. Source voice/page are immutable during the overlay; original open-HH page is retained for cancellation.

- Entry is unchanged: stopped transport, Voice page/mode, unshifted COPY+DATA, then release both. The first held press/release never confirms.
- DATA at Kit or Drum toggles that editor. Turns in an editor change its value within K01–K64 or D1–D3. Leaving editing does not transfer. Navigation turns select fields; only a fresh DATA rising edge at OK calls existing `preset_transferVoiceTo`.
- Physical `BUT_MODE1` cancels at every stage. It calls the local cancellation helper, restores the original source Voice page, clears normal edit mode/cursor and sets `SELECT_MODE_VOICE`; it bypasses normal VOICE-to-STEP handling. Held/repeated edges and releases are consumed, with a 40ms release guard. Normal VOICE-to-STEP outside the modal remains functional.
- COPY after entry has no dialog action. Press/release are consumed; COPY at OK does not obstruct the separate explicit DATA confirmation. Existing normal copy state/blink LEDs are cleared at entry by unchanged `copyClear_reset`. Held COPY continues to block post-dialog DATA draining until release; ordinary COPY works again outside.
- Instrument keys 1–7 cancel/consume and never select a destination or retarget source. Held/repeated instrument edges/releases stay consumed after closure. Other unrelated presses retain safe cancel/consume policy; START/STOP passes to transport and running confirmation is rejected by the existing transaction.
- DATA edges are tracked locally; no click edge also processes a turn. Inputs drain until all held modal controls have been released and DATA is stable for 40ms. Notice strings stay `success!`, `rejected`, `Morph active`, or the retained unrelated-key `cancelled` notice; VOICE returns immediately without a cancellation overlay.

The confirmation call remains only in the OK branch:

```c
if(menu_voiceCopyFocus != VOICE_COPY_OK)
    menu_voiceCopyEditing = !menu_voiceCopyEditing;
else {
    uint8_t result = buttonHandler_getShift() ? PRESET_VOICE_REJECTED :
        preset_transferVoiceTo(&menu_voiceCopy, menu_voiceCopyTarget, menu_voiceCopyTargetVoice);
    /* finish with the existing truthful notice */
}
```

LCD uses fixed 16-byte rows, no formatting buffer/allocation. Row 0 is `copy voice to:  `. Examples below include their final padding cell; tests compare all 16 cells and adjacent guards:

- `D1 > K01 D1  OK `
- `D1 > [K01] D1 OK`
- `D1 > K01 [D1] OK`
- `D1 > K01 D1 >OK `
- `V4 > K01     OK `
- `V4 > [K01]   OK `
- `V4 > K01    >OK `

Navigation Kit/Drum uses the existing `VIS_CURS(1,5,1)` / `VIS_CURS(1,9,1)` cursor. Editing uses brackets; OK uses `>OK`. There is no potentiometer editing. Shared HH retains the existing normalized same-instrument transfer including both decays.

Changed nonproduction files: existing R21 host fixture (new UI-driving helpers, real mode getter/setter and VOICE-case extraction, real copy-reset extraction, transaction/LED observers), R22 cases adapted only for superseded UI, new `navigation_cases.c`, dedicated workflow naming and ARM validator updated for TEST02/exact TEST01 baseline. Existing transfer-oracle assertions were retained. Full patch and path list are in evidence; 8 files, +400/−104 lines.

## Validation actually executed

All 11 local regression commands in `run-ledger.json` / `regression-results.json` exited 0: R21 voice copy, R22 Drum copy/navigation, project storage, project transaction/recovery, R20 UI, R20 full integration probe, Delay mono, USB-MIDI, Compressor mono, R18 CLP compare, BUG001 parameter sweeps. R21/R22 fixtures use ASan/UBSan and actual production transfer/selector/public panel/encoder paths with actual FatFs project roundtrips. Hardware and unrelated helper boundaries remain adapters; this is not a whole-device emulator.

Two additional local C11 `-O2 -Wall -Wextra -Werror -fsanitize=undefined,bounds -fno-sanitize-recover=all` builds/runs passed: BUG008 capture and FIX8 playback. FIX8 checked all 16,384 start/end pairs, identical intermediate events and 106,792 changes. CI independently reran these tests and the adaptive-removal/source assertions before ARM Release.

Focused TEST02 coverage:

| Check | Actual result |
| --- | --- |
| Seven source pages, all entry blockers, initial held chord, sub-40ms DATA bounce | PASS |
| Kit→Drum→OK and reverse, bounded navigation/edit values, K01/K64, D1/D3, non-Drum omission, source immutability | PASS |
| Exact 16-cell rows, cursor values, no embedded NUL/adjacent write, no premature transaction/RAM/engine/pattern/SD effects | PASS |
| VOICE during held entry/Kit/Drum navigation/edit/OK across source types, open HH, repeat/hold/bounce, post-release DATA | PASS; original page/mode, no STEP or transfer; fresh normal VOICE positive control enters STEP |
| COPY repeat/hold/releases across focus/edit stages; no cancel, stale blink/copy state or track copy; normal COPY restored | PASS |
| All 7 instrument keys across stages, held/repeated/releases | PASS; cancel/consume, source/target unchanged |
| Explicit Kit 2→Drum 3 OK with COPY held, DATA hold/bounce/release | PASS; exactly 1 real transaction, zero FX/MIDI/front-panel/SD dispatch at transfer |
| Same-kit all 9 rejects, invalid snapshot/type/context, pending pattern/kit, source identity, STEP base capture/release | PASS (inherited assertions retained) |
| Original Morph 57→123 plus remapped nonzero/zero-pending/zero-idle/unrelated/commit-boundary cases | PASS, truthful rejection and data/state preservation |
| All 9 cross-kit pairs, independent 25-field menu oracle, eligible source LFO override/remap/exact conflicts, all 306 eligible LFO targets | PASS |
| Shared HH/both decays, source/destination isolation, unsaved source, real project/temp save/reload, normal/immediate kit load | PASS |
| Outside DATA edit/navigation, COPY/CLEAR, REC/STEP, instrument and VOICE/PERF dispatch, notices/debounce | PASS at actual public dispatch boundary; unrelated mode helpers remain adapters except real VOICE case |
| Outside save/load name editing | PASS: modal harness verifies unchanged public routing; supplemental extracts full real unchanged save/load handler and name renderer, checks character navigation/edit/exit/OK/back, cursor/brackets, bounds and slot selection |

The supplementary name-editor probe forbids storage/engine adapters and never invokes them. Its initial fixture compilation failed because of missing/mismatched declarations and include/order setup; fixture-only corrections resolved it. The raw error is retained as `name-editor-first-fixture-error.log`. No firmware behavior assertion, mandatory regression or ARM build failed; no CI rerun or second candidate build was needed. Early development R21/R22/navigation runs also passed and their logs are retained. No tests were relaxed to change the production preset/LFO/Morph behavior. A report-only whitespace scan flags preserved original log blank lines, diagnostic caret spacing and patch context; raw evidence was kept byte-for-byte. The source commit whitespace check passed.

## ARM Release CI and archive

- [Run 43647784692](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43647784692), attempt 1, [job 132609298661](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43647784692/job/132609298661), **success** on exact source `4372e60337bf5ce35d67e8b5eb577baa3fffb962`.
- Workflow `.github/workflows/r22-drum-copy-validation.yml`; push to `release/r22`. GitHub run metadata retains its prior workflow display name; the checked-out TEST02 workflow and artifact naming are verified at exact source.
- One candidate ARM build: `cmake --preset release`, `cmake --build --preset release --parallel 2`. Matched compiler `arm-none-eabi-gcc (Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174)) 14.3.1 20250623`; toolchain archive SHA256 `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd` checked in CI. Build log has no warning/error matches.
- [Artifact 14134945492](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43647784692/artifacts/14134945492): `r22-test02-navigation-build-evidence`, original ZIP 755,900 bytes, created 2026-11-14T22:23:44Z, expiry **2026-12-14T22:23:43Z**, retention 30 days. Original archive downloaded and digest/CRC checked, not repacked.
- Archive SHA256: `e60110358b356a3cbb1a947c1d78c9545659db62d7348095ea349395a0a2a435`. Local retained original: `/workspace/deliverables/LXR_R22_TEST02_CI_Evidence_43647784692.zip`.
- Archive contains original candidate IMG and one-image ZIP, ELF, MAP, compiler/section data, validation manifest and all workflow logs. Permanent binary storage: **NOT ARCHIVED** (no verified permanent binary destination). Public Git report evidence contains text only; no firmware binaries/private KnowledgeBase.

| CI output | Bytes | SHA256 |
| --- | ---: | --- |
| `LXR_V2_R22_TEST02.img` | 297696 | `f3a39aa792cc7b54ae9e260bdc6bedd45dbbb28934da43035228b26f7e58c9f3` |
| `LXR_V2_R22_TEST02.zip` | 213653 | `9f85a4b4fdb9b037837b5291b105894b87ce69fa8862cf28753c301846b0c82c` |
| `LXR_V2_STM32F765.elf` | 404496 | `a4ad4f3ef852f8c8f210c9016ad9ad7f9d7c28b2671a5000fddc83227aacca5c` |
| `LXR_V2_STM32F765.map` | 552877 | `690ac84e785bff9e364fa6c614e9bfdfdf2d943c1cd2478118e1745c5b791e94` |

Delivery verifier independently checked each byte count/hash against the manifest; IMG updater magic `LXRV2IMG`, payload length 297,680, checksum byte 136 (`0x88`, `255 − sum(payload) mod 256`), reserved bytes 0; boot bytes `4c5852204c41422052323200` (`LXR LAB R22\0`) at IMG offset 161,552; all file-backed ELF PT_LOAD segments equal IMG payload at physical load addresses; vector SP alignment/range and Thumb reset entry checked by CI. The maximum load extent matches full IMG payload.

`LXR_V2_R22_TEST02.zip` contains **exactly one** `LXR_V2_R22_TEST02.img`, unchanged original internal filename, valid CRC, bytes identical to standalone IMG. Delivered IMG/ZIP are exact copies of CI outputs; the ZIP was not regenerated.

## Matched resource and stack comparison

Exact TEST01 and TEST02 artifacts use the same ARM GNU14.3.Rel1 CMake Release configuration. Region spans include alignment gaps and reserved heap where applicable; DTCM headroom is before runtime stack usage.

| Quantity (bytes) | TEST01 | TEST02 | Delta | TEST02 static headroom |
| --- | ---: | ---: | ---: | ---: |
| .text | 159856 | 160728 | +872 | — |
| .rodata | 83168 | 83272 | +104 | — |
| ITCM | 15304 | 15304 | 0 | 1080 |
| DTCM | 107424 | 107424 | 0 | 23648 |
| SRAM | 275156 | 275164 | +8 | 118052 |
| FLASH_payload | 296704 | 297680 | +976 | 718128 |

IMG length including 16-byte updater header: 296,720→297,696 (+976). `.bss` grew 8 bytes (273,788→273,796); `.data` remains 852, reserved heap 512. The existing 247-byte transfer staging array/snapshot architecture is unchanged. No audio-boundary command, live-engine LFO application or modulation engine change was introduced. Static increases do not exhaust any linker region; linker/validator pass. This is not a runtime safety proof.

Observed fixed entry frames (register saves plus immediate SP subtraction), decoded from both actual ELF/IMG pairs:

| Function | TEST01 bytes | TEST02 bytes | Delta |
| --- | ---: | ---: | ---: |
| `preset_transferVoiceTo` | 320 | 320 | +0 |
| `menu_parseEncoder` | 40 | 40 | +0 |
| `menu_voiceTransferButton` | 16 | 24 | +8 |
| `menu_repaint` | 88 | 88 | +0 |

`menu_parseEncoder` / `menu_repaint` contain the inlined selector encoder/renderer. `menu_voiceTransferButton` saves two more registers (+8); transfer frame remains 320 (36 saved +284 local). The encoder remains 40, renderer/repaint 88; the button entry changes 16→24. `stack-entry.json` retains exact prologue bytes, addresses, register masks and ELF identities. These are fixed entry comparisons, **not** whole call-tree bounds: later paths, callers/callees, interrupts and actual stack high-water are excluded.

**UNMEASURED:** CPU/WCET, IRQ/DMA deadlines, stack high-water, long-duration reliability, physical panel/display behavior and acoustic/audio results. Source and host preservation plus successful ARM linking do not establish those properties. R18 accepted CLP sound and R20 stable behavior remain protected references, not newly certified physical findings.

## Evidence, publication and next owner decision

[Text evidence](../evidence/r22-voice-copy-navigation-test02/README.md) includes full additive patch, source path list, immutable boundary hashes, focused UI cases, exact commands/results, CI run/job/artifact manifests, raw logs, binary verification and stack decoder. Candidate/source and report publication are authorized by this task; no release, merge, flash or device installation was performed. Source was independently fetched at its exact remote URL and blob identity. Report publishing is checked against remote branch/file content after commit.

**NEXT OWNER DECISION:** review this TEST02 UI candidate/evidence and decide whether to authorize a separate device validation task with the required backup/recovery arrangements. There is no current installation authorization or stability acceptance.

For a separately authorized future device session: verify entry/release; DATA Kit edit/exit then Drum edit/exit and explicit OK (no earlier transfer); VOICE cancellation from each stage without STEP; ignored COPY plus normal outside COPY; same-kit/Morph/running rejections; project save/reload and protected sound comparison. This checklist is proposed, **NOT RUN**.
