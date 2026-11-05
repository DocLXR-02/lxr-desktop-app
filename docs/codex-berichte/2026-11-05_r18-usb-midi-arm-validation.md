# R18 USB-MIDI — ARM Release Validation Build Report

**Date:** 2026-11-05 (Europe/Vienna)  
**Status:** **PASS — authorized, isolated ARM Release validation build; NOT a firmware delivery, release, installation or hardware validation.**  
**Task authorization:** The owner explicitly approved the next USB-MIDI ARM validation after the documentation synchronization. No production-code modification, CLP retuning, Delay/Compressor integration, flashing, device tests, source merge or release was approved or performed.

## Source identity and protection

| Role | Immutable identity |
|---|---|
| Repository | DocLXR-02/lxr-desktop-app |
| Accepted, unchanged CLP R18 reference | **41b364067bce0d8cde624c184933d9cb88e2725b** (fx/clp-v21-200hz-soft-r18) |
| Exact previously host-verified USB-MIDI source / parent | **b800669caf209067c47447fb6376abfbe7eaa7b3** (fix/r18-usb-midi-ring-ownership) |
| Separate ARM validation branch | validation/r18-usb-midi-arm-2026-11-05 |
| **Actual ARM-built commit** | **c5d5d8fa025506b1a9a85b10ebffd0934a38b027** |
| Only files added beyond the host-tested USB parent | .github/workflows/r18-usb-midi-arm-validation.yml; tests/r18_usb_midi/validate_arm.py |
| Production USB-MIDI files different from R18 | Core/Src/Soundengine/USB/usb_manager.c; Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c; Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h |
| CI | [SUCCESS — run 42061249085](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42061249085), attempt 1, job **127363262328** |
| Evidence artifact | **13432680833**, r18-usb-midi-arm-validation-evidence; configured retention 30 days, expires **2026-12-05 16:06:40 UTC** |
| Documentation/report branch | reports/2026-11-05-r18-usb-midi-arm-validation, branched from canonical documentation commit **3cdca52d926c220c3bbcf2c5cc32bc05a5681452** |

The exact original USB candidate remains untouched. This task introduced only CI/build verification assets on a separate branch; no production DSP, audio mixer, FX bus, Delay, Compressor, main boot, accepted CLP reference, or original USB patch was changed.

## Executed checks and build

**Runner:** GitHub Actions ubuntu-24.04.  
**Matched toolchain:** Arm GNU Toolchain **14.3.Rel1**, arm-none-eabi-gcc **14.3.1 20250623**, CMake Release configuration. Toolchain archive SHA-256: **8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd**. This matches the accepted R18 comparison build toolchain and compiler command JSON.

All CI job steps completed successfully:

1. **Source-boundary preflight:** Checked exact candidate parent, ancestry from accepted R18, allowed changed paths, correct Git blob IDs for all three USB production files, and byte identity of CLP/Delay/Compressor/mixer/FX-bus/main code against R18. PASS.
2. **USB-MIDI host tests:** Ran python3 tests/r18_usb_midi/run.py build/r18-usb-arm-host. All **15 classified cases PASS their expected outcomes**, including the three deliberately reproduced original-fault abort cases. Candidate normal TX/RX, BUSY retry, external BUSY, in-flight buffer, wrap, disconnect/retry, RX full/drain and incomplete USB packet handling passed. SHA256SUMS in the host test output passed.
3. **Existing R18 CLP and unrelated regressions:** R18 CLP unit/state/mixer/FX-bus tests PASS; 75,000 differential mixer cases, 135 core-state snapshots, 16,384 MAX-control selector cases and 4,096 ramp checks PASS. BUG001, kit loading, project storage and Euclidean tests PASS. These are host tests, not Cortex-M7 real-time measurements.
4. **ARM build:** cmake --preset release; cmake --build --preset release --parallel 2 — both PASS. The archived build log contains no compiler warning/error messages.
5. **ELF/IMG validator:** python3 tests/r18_usb_midi/validate_arm.py — PASS. Verified R18 startup marker, LXRV2IMG signature, payload size/header checksum, reset and stack vectors, ARM Cortex-M ELF identity, four nonempty load segments and exact segment bytes in the IMG, FLASH address/range fit, pinned USB source blob IDs and resource comparison against archived R18 sections.
6. **Evidence retention:** Wrote a source ZIP, ELF, MAP, IMG (clearly designated validation-only), compile commands, resource JSON, logs and SHA-256 manifests; uploaded the GitHub Actions artifact. No firmware delivery ZIP was made.

There was **one successful CI attempt** for this branch and no failed target build. Earlier failed USB host-CI evidence-generation attempts belong to the already reported November 5 USB host task, not to this ARM run.

## Matched resource ledger — original R18 versus USB-only candidate

All sizes are **bytes**. Comparisons use the same ARM toolchain, Release settings and compiled configuration. The section-only baseline is from the accepted R18 raw build evidence, not an unrelated firmware version.

| Metric | Accepted R18 | USB-only ARM candidate | Difference |
|---|---:|---:|---:|
| Ordinary FLASH .text | 151,784 | 152,008 | **+224** |
| .bss | 273,280 | 273,296 | **+16** |
| ITCM .itcm_text | 15,208 | 15,208 | **0** |
| DTCM .dtcm + .dtcmz | 107,420 | 107,420 | **0** |
| General SRAM .data + .bss + ._user_heap | 274,644 | 274,660 | **+16** |
| Complete application FLASH payload | 287,928 | 288,152 | **+224** |
| Complete IMG including 16-byte header | 287,944 | 288,168 | **+224** |

**Remaining nominal allocations:** ITCM **1,176 B**; DTCM section-only gap **23,652 B**, before any real stack high-water; general SRAM section-only nominal gap **118,556 B**. The historical aggregate resource ledger adds four bytes to its static DTCM and general-SRAM totals; the **measured before/after deltas remain zero DTCM and +16 B general SRAM** under both counting conventions. No runtime stack, cycle, IRQ, DMA or acoustic cost can be inferred from static growth.

An independent review compared **all 18 section entries** from the archived original R18 section table against the candidate: only .text (+224) and .bss (+16) changed; other sections were identical in size. All **four ELF FLASH load segments** matched IMG bytes independently.

## Independently verified evidence and hashes

The CI artifact was downloaded and independently checked outside the build job. **ZIP CRC PASS, 33 entries, 5/5 primary hash-manifest entries and 32/32 record-hash entries PASS.** The source archive passed its own ZIP CRC check.

| Artifact | Size | SHA-256 |
|---|---:|---|
| Downloaded full GitHub CI evidence ZIP | **2,184,525 B** | **01973718fd0ffabad78d1502e74a359793ddf2ac9cf10510e3434cd83e51c9b1** |
| USB ARM validation-only IMG | **288,168 B** | **18653ded811d97afffda7747d1cd1ecd1ae23507fbe90bca2b5ff51626e545aa** |
| ELF | **391,916 B** | **8b2c1b0557eeeb668b6627aa277f8f52ce5583a364f030a632594bdf5d18aa38** |
| MAP | **541,616 B** | **4bcae5753ef51d83c0796244af194b81d1d10c66bd921df328b3ed5053ff577e** |
| SOURCE.zip | **1,734,540 B** | **1dbdf22023fd1fabb10d6471fbf8e0a28c261e2e176e7f6ac8068a7178da8098** |
| Original accepted R18 IMG (historical reference) | **287,944 B** | **c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1** |

**Independent source comparison:** The candidate source ZIP was compared byte-for-byte to the original R18 source ZIP preserved in the owner's previously supplied Delay ARM evidence package. **482 shared archive entries were byte-identical; exactly three production files changed, all in the USB-MIDI subsystem.** Exactly four workflow/test files were added, including the two added specifically for this ARM validation. No baseline source paths were removed. R18 and candidate compile_commands.json were byte-identical, and toolchain archive SHA-256 identities matched.

**Independent review bookkeeping:** The original R18 SHA256SUMS references the separately delivered R18 IMG/ZIP rather than embedding those two delivery files inside the nested reference evidence ZIP. The source-archive checksum was verified from the available bytes; delivery hashes remain historical report references. The reviewer also ignored a newly added ZIP directory entry when classifying added source files. These two archive-layout corrections affected only the local verification script, not the compiled source or CI outcome; final source/segment/hash comparisons passed. No redundant ARM rebuild was needed.

**Retention:** The raw CI ZIP is available via [run 42061249085, artifact 13432680833](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42061249085/artifacts/13432680833) until its GitHub Actions expiry. A byte-matched copy was placed in the owner's private Library at /LXR Firmware Lab/LXR_R18_USB_MIDI_ARM_Validation_Evidence_2026-11-05.zip. This copy is **not** published as a public GitHub binary archive and is not a guarantee of indefinite future retention. Source scripts and the workflow remain on the pinned validation commit.

## Open engineering risks and authorization boundary

**NOT EXECUTED / NOT MEASURED:** flashing, installation, USB transport on actual LXR-02 hardware, stress testing of USB IRQ/cache/DMA concurrency, simultaneous heavy MIDI/audio traffic, Cortex-M7 CPU cycles and WCET, audio deadlines, stack high-water, soak/thermal reliability, or device listening. The original R18 CLP sound remains owner-accepted but this candidate has not been compared acoustically on hardware.

**USB transport limitations remain explicit:** At capacity TX holds at most 31 queued four-byte events and RX 63; newest complete events are dropped. Note-Off and clock can be dropped under sustained overload. Disconnect/reconnect retry is not exactly-once delivery. Host models and successful ARM compilation are not proof against target interrupt races or stale/underrun audio.

The **Delay mono fix** is an independent candidate at 08bc165500e21153eeb28ddbf2fe2d84626a82ca and was **not integrated**. **Compressor mono over-range conversion is not yet corrected** and needs a separate policy/sound decision and authorization. No integrated R18 successor, R19 release, merged source or installation permission follows from this build.

**Recommended next authorized decision:** A separate Compressor policy and isolated correction/test task, or a separate targeted on-device USB-MIDI test plan if the owner prioritizes hardware validation. Do not start either automatically. Preserve the accepted R18, original USB candidate, Delay candidate and protected refs.

## Reproducibility references

- [Exact USB validation source](https://github.com/DocLXR-02/lxr-desktop-app/tree/c5d5d8fa025506b1a9a85b10ebffd0934a38b027)
- [USB ARM CI workflow](https://github.com/DocLXR-02/lxr-desktop-app/blob/c5d5d8fa025506b1a9a85b10ebffd0934a38b027/.github/workflows/r18-usb-midi-arm-validation.yml)
- [USB ARM ELF/IMG/resource validator](https://github.com/DocLXR-02/lxr-desktop-app/blob/c5d5d8fa025506b1a9a85b10ebffd0934a38b027/tests/r18_usb_midi/validate_arm.py)
- [Earlier exact host-tested USB report](https://github.com/DocLXR-02/lxr-desktop-app/blob/e776e0d3b7c244c3cc0493a89ec3c37cd93e1e28/docs/codex-berichte/2026-11-05_r18-usb-midi-transport-ownership-host.md)
- [Successful USB ARM validation run](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42061249085)
- [Accepted R18 original build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1eee9ed318b592cec54379bcc255927968b936a3/docs/codex-berichte/2026-11-03_r18-clp-v21-soft-build.md)

**Publication scope:** Separate report-only branch based on the canonical documentation branch. This report does not rewrite historical reports, the current project status, the owner's private KnowledgeBase, accepted firmware or production source.
