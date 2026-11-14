# R20 automation target names and shared 0sr build

**Date** 2026-11-13 (Europe/Vienna)  
**Repository** `DocLXR-02/lxr-desktop-app`  
**Status** Host and ARM Release CI PASS. Physical validation of this candidate is pending.  
**Authorization** Owner approved the LCD name lookup and global `0sr` conflict corrections, regression tests through the panel and STEP message paths, and a new ARM CI build on a separate branch.

## Source and findings

- Approved base `f40eff9b8938f57cb7f1c719211a23b2da236dc1`, the previous corrective R20 test build. Its source and binary history are retained in [CI run 43399545352](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43399545352) and `LXR_R20_Fix_Build_Report_2026-11-13.zip`.
- Built commit [b01537282047bda5299cdb1bedeab5e5d5b9bcca](https://github.com/DocLXR-02/lxr-desktop-app/commit/b01537282047bda5299cdb1bedeab5e5d5b9bcca), parent exactly the approved base, tree `3d3eed7a3b7b2dd9064efd6871b0ac1da69eeea5`. [Branch](https://github.com/DocLXR-02/lxr-desktop-app/tree/fix/r20-target-names-2026-11-13)
- Inputs included active private KnowledgeBase v1.5.6, applicable source AGENTS.md, GitHub project status/workflow/evidence and recordkeeping, the previous R20 test/build records, and the owner observation of wrong displayed names.
- The verified remote diff is one commit with seven paths. Production code changed in `Core/Src/menu/menu.c` and `Core/Src/Soundengine/Sequencer/sequencer.c`. Remaining changes are host fixtures and the CI branch filter. The original R20 base, prior corrective R20, FIX8, R19, R18, R11, R4, and `main` remain protected. R18 CLP sound is closed. R19 is the approved stable firmware.

The low parameter path in CortexParser adds one to the panel parameter byte before passing the destination to the sequencer. The same representation is used by manual STEP. Playback consumes that stored number as its MIDI CC. The R20 LCD lookup previously used it as a panel ID. With production encoding, panel COA ID 8 became stored CC 9 and rendered FIN on both tracks. Panel `0sr` ID 114 became stored CC 115 and rendered FRQ. The pre-fix reproduction also accepted two cross-voice recordings of stored CC 115 because the shared-target switch tested panel ID 114. The previous integration fixture subtracted one when sending low panel IDs and so concealed the production offset.

`menu_autoShortName` now checks OFF and MUL first and converts a low stored CC back to a panel ID for name lookup. High IDs are passed through. This applies to the REC overview and local or shared rejection notices. `seq_isSharedAutomationTarget` now recognizes `VOICE_DECIMATION_ALL`, the stored MIDI CC. Existing duplicate policy still allows editing a previously stored duplicate in place. The change leaves stored values, STEP encoding, playback conversion, FIX8 interpolation, project format, bootloader and DSP unchanged.

## Validation

The pre-fix expanded fixture reproduced `REC trk1 FIN` for stored COA and exited with an LCD assertion. The earlier diagnostic trace records the corresponding locked-track notices and the accepted `0sr` duplicate. Both logs are included in this report ZIP. During local fixture maintenance the old assertion counting only `PAR_` switch cases failed when the MIDI CC constant was introduced. The fixture now counts all named cases. Local `python3 tests/r20_ui/run.py`, `python3 tests/r20_ui/integration/probe.py . ../target_names_host`, and `git -c core.whitespace=cr-at-eol diff --check` passed on the final source tree.

The integration fixture reuses the production panel send block, STEP menu dispatch, CortexParser handlers, capture engine, sequencer and LCD buffer refresh. It verifies the persisted destination byte and full LCD rows for nine targets on both tracks through REC and STEP (36 combinations). Tests include COA, FIN, filter FRQ, ATK, DEC, global 0sr, voice-local LFO frequency, filter drive at the 128 boundary and FX P4. It verifies `0sr` conflicts via STEP, the sibling lane and a separate voice after a real simulated loop commit. A voice-local LFO frequency at the adjacent low CC is allowed independently. Existing tests exercise all 12 shared targets, pending reservation, MIDI interleaving, notice expiry and wrap, STEP cache resync, context changes and blocked arming LED behavior. Physical panel IO and target timing are outside the host fixture. The inherited REC LED behavior after context invalidation remains separately documented.

[GitHub Actions run 43404559276](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43404559276), attempt 1, job `131815595871`, **SUCCESS**. Arm GNU Toolchain 14.3.Rel1 (GCC 14.3.1), CMake Release. All host regression steps passed, including FIX8 parity over 16,384 endpoint pairs and 106,792 intermediate changes, project storage and transaction, Delay, USB-MIDI, Compressor and CLP. The ARM build log had no compiler warning or error matches. The original CI evidence contains the source manifest, compiler/build logs, host outputs, ELF, MAP and image. GitHub artifact `14026826459` is scheduled to expire on 2026-12-13. The unmodified artifact ZIP is retained as a separate deliverable.

## Verified outputs

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R20_NAMES_TEST.img` | 293,504 | `27ebacee348b8dd42d0a2cf35d1513676c712583c2891559483a8a47353e0056` |
| `LXR_V2_R20_NAMES_TEST.zip` | 210,077 | `13c81c9615d83774f34a69eb6d6e12d1682e04ba211692aa4a241b035970bd34` |
| `LXR_V2_STM32F765.elf` | 398,780 | `74266957967a74c1971f18995f56b0b0ad643d08f9a9f76d9e4257b3aa736512` |
| `LXR_V2_STM32F765.map` | 549,323 | `9d99d0f2478eadd749b09b0e693c4f6ad1dd50406b66c4d62d7ddeda9898c3c3` |
| `LXR_R20_Names_CI_Evidence_43404559276.zip` | 531,412 | `ac369072d615d7c7ee478eea0b7c2cbcfc7ef800be0824e07f1211a20d2d7d6b` |

Verified exact source SHA in the evidence manifest, CI archive CRC and SHA-256, ELF/MAP/IMG manifest hashes, the single-entry firmware ZIP CRC and byte-for-byte IMG equality to CI. Updater magic `LXRV2IMG`, payload length 293,488 B, additive checksum byte 0x35 and startup label `LXR LAB R20` passed.

| Static bytes | Previous corrective R20 | Name correction | Delta |
|---|---:|---:|---:|
| Flash `.text` | 157,056 | 157,064 | +8 |
| ITCM | 15,304 | 15,304 | 0 |
| DTCM occupied span | 107,424 | 107,424 | 0 |
| General SRAM occupied span with reserved heap | 274,884 | 274,884 | 0 |
| Flash load payload | 293,480 | 293,488 | +8 |
| Updater IMG | 293,496 | 293,504 | +8 |

Nominal gaps remain 1,080 B ITCM, 23,648 B DTCM before runtime stack, and 118,332 B SRAM. CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain **UNMEASURED**. No flash, physical test, merge or public release was performed. The separately authorized device validation can compare a COA-bound track and a 0sr-bound track with the prior owner observation and verify a competing 0sr assignment. This is a private build report, separate from the published firmware source branch.
