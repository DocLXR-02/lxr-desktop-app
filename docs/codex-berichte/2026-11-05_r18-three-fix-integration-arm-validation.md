# R18 Three-Fix Integration — Joint Host and ARM Release Validation Build Report

**Date:** 2026-11-05 (Europe/Vienna)  
**Status:** **PASS — integrated technical validation only. NOT OWNER-ACCEPTED FIRMWARE, NOT FOR INSTALLATION.**

## Owner authorization, objective and exclusions

After separate successful Delay, USB-MIDI and Compressor host/ARM validations, the owner expressly approved **a new branch derived from the immutable accepted R18 source**, integrating only these three existing production corrections, then executing joint host and ARM Release checks with a separate build report. The owner previously approved the signed 16-bit saturation policy for Compressor mono over-range behavior.

This authorization **does not** authorize merging into main or accepted R18, creating a release, flashing/installing the validation image, changing CLP sound, modifying the bootloader, or performing unplanned hardware tests. The physical musical acceptance of the new integrated candidate is **OPEN**; the separate accepted FX-CLP R18 sound reference remains closed and unchanged.

This task used direct CHAT orchestration of GitHub and CI. Codex/Work was unnecessary for this narrowly bounded integration of already verified, non-overlapping source files; no live agent quota data was available and no quota cost is invented.

## Exact source identities and independent provenance

| Role | Identity |
|---|---|
| Repository | DocLXR-02/lxr-desktop-app |
| **Original owner-accepted R18 source** | **41b364067bce0d8cde624c184933d9cb88e2725b** (fx/clp-v21-200hz-soft-r18) |
| Original accepted R18 application IMG | LXR_V2_R18.img, **287,944 B**, SHA-256 **c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1** |
| Delay individual ARM-tested candidate | **08bc165500e21153eeb28ddbf2fe2d84626a82ca** |
| USB-MIDI individual ARM-tested source candidate | **b800669caf209067c47447fb6376abfbe7eaa7b3** (validation child c5d5d8fa025506b1a9a85b10ebffd0934a38b027) |
| Compressor individual ARM-tested source candidate | **ff8f7ae5a7217f65928318f23e8c69b1e6a22485** (validation child 3b86e2bcd87fd48eb1de9f9e66650d3583650448) |
| **New integrated experimental branch** | **integration/r18-delay-usb-compressor-validation-2026-11-05** |
| **Exact integration commit and ARM-built source** | **cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6** — direct child of the accepted original R18 source, NOT a merge of historical branch commit chains |
| Successful joint GitHub Actions | [run **42069839191**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191), attempt **1**, job **127394415842**, completed **SUCCESS** |
| GitHub Actions evidence | Artifact **13436855057**, r18-three-fix-integration-arm-validation-evidence; 30-day retention, expires **2026-12-05T16:58:26Z** |
| Separate report-only branch | reports/2026-11-05-r18-three-fix-integration-arm, derived from canonical documentation commit **3cdca52d926c220c3bbcf2c5cc32bc05a5681452** |

Exactly **five production files** differ from original accepted R18, and every modified file is byte-identical to its separately tested source candidate:

| Production file | Exact reused source Git blob |
|---|---|
| Core/Src/Soundengine/DSPAudio/FX/Delay.c | **1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6** |
| Core/Src/Soundengine/USB/usb_manager.c | **7f30ed0fcdb433f280b98bb238879cb28834b585** |
| Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c | **ef33e3e2941db2a8a72705d66d419c29966f4d47** |
| Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h | **56bb9165ae5d2bf0134a55a4a95652bc706aef77** |
| Core/Src/Soundengine/DSPAudio/FX/Compressor.c | **7bb84666575c0a35e93e10563aa5b5c559432030** |

Five **new non-production files** were added: three reused, source-pinned host runners at tests/r18_delay_mono/run.py, tests/r18_usb_midi/run.py and tests/r18_compressor_mono/run.py; one new combined GitHub Actions workflow at .github/workflows/r18-integrated-arm-validation.yml; and one combined ELF/IMG/resource validator at tests/r18_integration/validate_arm.py. No original source files were deleted.

The integration commit's GitHub comparison confirms **exactly these ten file paths**. The integration CI separately checked the exact R18 parent commit, all allowed paths, all five production Git blob IDs and Git diff whitespace. Neither CLP production code nor mixer/FX-bus/boot source was changed.

## Reused history and actual joint host validation

The input fixes and their limitations were documented independently:

- Delay: corrected aligned 16-bit mono read/write in normal and ping-pong operation; [isolated ARM report](https://github.com/DocLXR-02/lxr-desktop-app/blob/bdbe8857e16eda7cecf0bfdeee048cede6ff6297/docs/codex-berichte/2026-11-05_r18-delay-mono-arm-validation.md).
- USB-MIDI: corrected CDC TX ring wrap, transfer BUSY and in-flight ownership, RX queue full/empty ambiguity and bounded overflow behavior; [isolated ARM report](https://github.com/DocLXR-02/lxr-desktop-app/blob/25cd2946ad36c363b6ffe5689045519ec4bed3dd/docs/codex-berichte/2026-11-05_r18-usb-midi-arm-validation.md).
- Compressor: corrected unchecked mono float-to-int16 output with owner-approved, stereo-consistent signed 16-bit saturation; [isolated ARM report](https://github.com/DocLXR-02/lxr-desktop-app/blob/3f17c2cdeaffd52ed4c53cdab5c0b4ec0221561f/docs/codex-berichte/2026-11-05_r18-compressor-mono-arm-validation.md).

**Executed in the combined source branch — all passed:**

1. Delay test runner reproduced the original R18 mono-right alignment sanitizer failure as **expected**, then passed the corrected normal/ping-pong mono-left/mono-right tests. **144 output comparisons** were performed; original stereo and safe mono-left outputs remained bit-identical under the fixtures.
2. USB-MIDI test runner passed **15 classified original-versus-candidate host cases**, including the original TX BUSY loss, out-of-bounds wrap and RX full-as-empty reproduction, and corrected capacity, retry, in-flight ownership, disconnect and short-packet cases. Its source-extracted shims do not emulate hardware USB timing.
3. Compressor test runner passed **10 classified host cases**, reproducing original positive and negative float-cast-overflow failures as **expected** and validating corrected saturation. The original and corrected samples matched **byte-for-byte over 144 bounded in-range channel/ratio/gain/dry-wet configurations**; this does not establish universal acoustic identity.
4. Existing R18 CLP and other regressions passed: CLP unit/state/mixer/FX-bus, **75,000 differential mixer evaluations**, **135 core snapshots**, **16,384 MAX selector cases**, **4,096 ramp checks**, BUG001, kit load, project storage and Euclidean suites.

Original fault reproductions intentionally cause sanitizer failures inside their expected-failure cases; they do **not** mean the integrated CI job failed. The successful CI distinguishes expected original failures from corrected-source passes.

## Matched ARM Release build and firmware structure verification

Environment: GitHub Actions ubuntu-24.04; Arm GNU Toolchain **14.3.Rel1**, arm-none-eabi-gcc **14.3.1 20250623**, archive SHA-256 **8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd**. Original R18 and integrated candidate compile_commands.json files are **byte-identical**; the toolchain archive identity is the same as the original archived R18 run **41725066117**.

Executed:

    cmake --preset release
    cmake --build --preset release --parallel 2
    python3 tests/r18_integration/validate_arm.py

All commands completed successfully. The compiled Release build log contained **zero compiler warning/error lines**. The candidate ARM ELF and updater-format IMG passed signature, 16-byte container header, payload-length and internal-checksum validation; the reset and initial stack vectors, ARM ELF identity, application FLASH address range, **four nonempty load segments** and exact segment bytes in the IMG all passed. The validation-only IMG retains the original startup string "LXR LAB R18": this is **not a new approved R18 image** and must not be installed on that basis.

### Matched static resource accounting

All entries below are **bytes**. The baseline is the independently archived, matched Arm GNU R18 Release build.

| Resource | Original R18 | Integrated 3-fix candidate | Difference |
|---|---:|---:|---:|
| Ordinary FLASH .text | 151,784 | 152,024 | **+240** |
| ITCM .itcm_text | 15,208 | 15,208 | **0** |
| DTCM .dtcm + .dtcmz | 107,420 | 107,420 | **0** |
| General SRAM .data + .bss + ._user_heap | 274,644 | 274,660 | **+16** |
| Total application FLASH load payload | 287,928 | 288,168 | **+240** |
| Full updater IMG including 16-byte header | 287,944 | 288,184 | **+240** |

Independently parsed **18 source/baseline linker section sizes**; exactly **.text +240 B** and **.bss +16 B** changed. All other sections retained their original sizes. The historical region-aggregate ledger includes four additional bytes in DTCM and general SRAM compared with the section-only grouping above; both conventions yield the same delta. Nominal remaining **ITCM 1,176 B**, **DTCM 23,652 B** (section-only, **not measured stack reserve**) and **general SRAM 118,556 B** (section-only). No ITCM or static DTCM growth was detected.

The combined flash delta **+240 B** equals the two individually measured changes (Delay +16 B, USB-MIDI +224 B, Compressor +0 B); SRAM delta **+16 B** equals USB-MIDI's isolated static growth. This is **a matched static build result**, not proof of timing or musical transparency.

## Independent source/archive verification and hashes

The entire GitHub Actions artifact was downloaded and independently inspected after the CI job completed:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| **Whole evidence ZIP** | **2,191,549** | **a86e3b093710548737ee9c033a515211eec947832eb2ea9e6b6fe791fab687b8** |
| **Integrated ARM validation-only IMG** | **288,184** | **50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7** |
| ELF | **391,916** | **ac75c8aa650eaf3fd166b9d88ecbb34a6679ed79577d24a5f7887e64c1132e6d** |
| MAP | **541,577** | **63e649b2130ae839c7ff166b7cfc45b06c75518d64747ab1539e35dddc29ed95** |
| SOURCE.zip | archived with the build | **c44b7581e5e92fe46bd0d06014e8886a0f0d17686b391f93fcf0bcd0f5404364** |

**Independent checks:**
- Outer evidence ZIP: **35 entries, ZIP CRC PASS**.
- The first SHA-256 manifest VERIFIED_HASHES.txt: **5/5 PASS**.
- CI record manifest RECORD_HASHES.txt: **34/34 PASS**.
- Inner SOURCE.zip and original accepted R18 SOURCE.zip from the owner's retained R18 reference build package: **ZIP CRC PASS**.
- Original-versus-candidate source archives: **399 shared files**, exactly **5 changed production files with five pinned candidate blob identities**, **394 byte-identical original files**, **5 added build/test files**, **0 removed original files**.
- Original R18 versus candidate compiler command JSON: **byte-identical**. Toolchain archive digests: **identical**.
- Four ELF load-segment payloads: byte-for-byte identical to the appropriate IMG addresses; internal IMG signature/length/checksum and accepted R18 boot marker checked.
- Original-versus-candidate 18 linker section sizes: exactly .text +240 and .bss +16; no unexplained size changes.

The GitHub-hosted artifact is available at [run 42069839191 / artifact 13436855057](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191/artifacts/13436855057), subject to **30-day** expiry. A copy was also uploaded to the owner's private Library at /LXR Firmware Lab/LXR_R18_Three_Fix_Integration_ARM_Validation_Evidence_2026-11-05.zip. Neither archive is a release package. Both contain a validation-only IMG marked NOT_FOR_FLASHING. The report and pinned source/workflow are retained on GitHub; the binary artifact is **not** published as a durable public repository release.

## Important limitations and next approval gate

**NOT RUN / NOT AUTHORIZED:** LXR-02 installation/flashing, physical listening comparison against the accepted R18, USB-MIDI physical host/DAW transport validation, Delay/Compressor hardware operation, integrated soak/stress tests, public release, bootloader changes, source merge into main/R18 or reassignment of the accepted R18 identity.

**UNMEASURED:** Actual Cortex-M7 audio CPU utilization/WCET, IRQ/DMA deadline margin, underrun/overrun counters, live stack high-water, USB ISR/cache concurrency behavior, long-term reliability, thermal stability, physical audio behavior and actual device signal clipping.

**Known finite USB behavior remains:** TX ring capacity up to 31 complete four-byte events and RX capacity up to 63 events; new events are intentionally dropped at capacity, including possibly Note-Off or clock messages. Reconnect retransmission is not guaranteed exactly once. These design limitations were not eliminated by integration.

**Compressor policy versus sonic acceptance:** The owner approved signed 16-bit saturation of over-range mono outputs, but the change could alter real output compared with previously undefined over-range behavior. No new full-device musical reference or accepted listening result is recorded.

**Next bounded decision:** Prepare a separate owner-approved device test and recovery/backup procedure for the integrated experimental candidate, followed by installation only if explicitly authorized. Test Delay mono-left/mono-right and ping-pong, USB-MIDI sustained/burst/disconnect traffic, Compressor mono high make-up gain in a controlled listening comparison, and unchanged accepted FX-CLP sound. Without real-time profiling tools, CPU deadlines, IRQ/DMA margins and stack high-water must remain labeled UNMEASURED. Do not automatically promote to R19 or merge into the frozen reference.

## Immutable GitHub pointers

- [Exact combined integration source](https://github.com/DocLXR-02/lxr-desktop-app/tree/cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6)
- [Original R18-to-integrated diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/41b364067bce0d8cde624c184933d9cb88e2725b...cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6)
- [Combined ARM workflow](https://github.com/DocLXR-02/lxr-desktop-app/blob/cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6/.github/workflows/r18-integrated-arm-validation.yml)
- [Combined resource and image validator](https://github.com/DocLXR-02/lxr-desktop-app/blob/cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6/tests/r18_integration/validate_arm.py)
- [Successful integrated CI run 42069839191](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191)
- [Canonical current project status — historical snapshot pending optional documentation sync](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/PROJECT_STATUS.md)

**Publication:** Dedicated documentation report branch. This record does not itself update canonical project status, overwrite the private KnowledgeBase or accept the candidate for physical installation.
