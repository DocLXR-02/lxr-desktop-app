# R23 MIDI Hardening — Validation Build and Evidence Closeout

**Date:** 2026-11-18  
**Status:** EXPERIMENTAL HOST/ARM VALIDATION PASSED; NOT A RELEASE; NOT APPROVED FOR FLASHING  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Owner authorization:** Six bounded MIDI safety/behavior corrections, targeted host verification, inherited regressions and ARM validation were approved in conversation; the owner subsequently requested closure reporting, archive retention and a private KnowledgeBase update. This documentation-only publication does not authorize a merge, device flash, new firmware edit, or release.  
**Exact firmware base:** `256497e7cd7b0070dc6fabc34c4f73f1712c4862`  
**Firmware branch:** `experiment/r23-midi-hardening-2026-11-18`  
**Final verified firmware SHA:** `5e99e4f57499e0946c90546d5b1fc6f1867bebd6`  
**Documentation-only base:** `672891c23809e8a2014457f564c16732f970e4f9` on `docs/lab-project-state`  
**Documentation branch:** `reports/r23-midi-hardening-closeout-2026-11-18` (separate from all firmware branches)

## Implemented corrections

1. Reject unsupported external `CC2_FX_TYPE` values before updating the CC2 cache, selected FX editor, or parameter banks; bound FX menu text lookup and sanitize invalid archived selected FX types.
2. Wrap MIDI CC0 Kit values 0–127 to one of 64 RAM Kit slots *before* changing the selected Kit number. Consistent with Erica Synths' official correction `dca0a44413931a12de65941355f06c07598a9be5`.
3. Process realtime MIDI bytes `0xF8..0xFF` independently without clobbering a partially received UART message or running status.
4. Remove the duplicated outgoing MIDI Note On from `voiceControl_noteOn`; preserve the sequencer's own Note On.
5. Correct the 1-based CC voice number to 0-based MIDI channel-array index.
6. Exclude CC0 and NRPN selector/control CCs from recorded step automation in both channel-filter branches.

**Production paths:** `Core/Src/frontPanelParser.c`, `Core/Src/Soundengine/MIDI/MidiParser.c`, `Core/Src/Soundengine/MIDI/MidiVoiceControl.c`, `Core/Src/menu/menu.c`, and `Core/Src/Preset/presetManager.c`.

**Test/CI paths:** `tests/r23_midi_hardening/run.py`, `tests/r23_legacy_kit_migration/run.py`, `tests/r21_voice_copy/host.c`, `.github/workflows/r23-midi-hardening.yml`.

Four FX DSP algorithm bodies, R18 CLP sonic logic, Delay tail freezing/resuming, FX route and Kit binary format, bootloader, protected `main` and accepted `release/r22` were **not modified**.

## Successful validation

[GitHub Actions run 44386179214](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44386179214), attempt 1, **SUCCESS**, matches the exact final source SHA. Host job `135091278152`: SUCCESS. ARM job `135091277722`: SUCCESS. Ubuntu 24.04; pinned Arm GNU 14.3.rel1 with download SHA-256 verified in the workflow.

- New extracted-production-source MIDI tests: **879 cases under UBSan and 879 under ASan; PASS**. These probe CC0, FX type range, menu/stored Kit bounds, note dispatch, per-voice CC channel selection, MIDI realtime state preservation, and automation selectors.
- Legacy Kit migration: **786,432 voice cases and 131,072 Kit layouts per sanitizer; PASS**.
- FX bank isolation: **246,615 checks per sanitizer; PASS**.
- Real four-DSP shared-FX integration: **5,832 six-voice scenes per sanitizer** (including **1,296 four-FX scenes); PASS**.
- Inherited R18 CLP, Delay mono/ping-pong state retention, Compressor, USB MIDI, R21/R22 copy, Kit/project FAT16/FAT32 and transaction recovery: **PASS**.
- Source-pinned ARM Release configure, compile/link, section limits, ELF/MAP/IMG hash generation: **PASS**. The IMG was **not** included in the distributable evidence bundle.

### Static ARM resource comparison

| Section / group | Pre-MIDI R23 base | Final MIDI R23 | Delta |
|---|---:|---:|---:|
| Flash `.text` | 172,648 B | 171,904 B | −744 B |
| Flash `.rodata` | 83,304 B | 83,304 B | 0 B |
| ITCM code | 13,600 B | 13,600 B | 0 B |
| DTCM static | 109,324 B | 109,324 B | 0 B |
| General SRAM static | 276,952 B | 276,952 B | 0 B |

Nominal static gaps: **ITCM 2,784 B; DTCM 21,748 B before stack; general SRAM 116,264 B**. These are linker-derived values, not measured CPU, realtime deadlines, or stack high-water.

**CI-only image identity:** `LXR_V2_V1.7.img`, **307,200 B**, CI-reported SHA-256 `0d79abe3319f3b0b4f7f1069a7870dcb9809c9a6d7db76f97acf44b2665d441b`. Image bytes were not independently retrieved or rehashed. **Do not use for installation.**

**Verified evidence artifact ZIPs** (exact original files preserved as nested ZIPs in the owner-held evidence archive):

| Artifact | GitHub ID | Bytes | SHA-256 |
|---|---:|---:|---|
| R23 MIDI final HOST | 14465112372 | 608,721 | `a10d125c71e6a37ba8531859deb3789da3a9e67741a09b146202b4fa42a67e40` |
| R23 MIDI final ARM | 14466506807 | 312,751 | `3ff5f303c91823e036b101e737fa012792f538ea02a16a98a3151ea74cebb500` |

The **original standalone, English, per-build report** is stored as `LXR_R23_MIDI_Hardening_ARM_Validation_Build_Report_2026-11-18.md` **inside** the owner-held archive `LXR_R23_MIDI_Hardening_Validation_Evidence_2026-11-18.zip` (ZIP SHA-256 `c20ce2d7f4d13104d7330c1bc5cf5945e4aefbd74094b02759f1e216ab2e30a5`; embedded report SHA-256 `53f670598e04c8e65f79861023b5e34dcbff908340449ddda7533e5413bbdc2a`). This public closeout record references the original report; it is **not** a separate firmware build or a replacement for that immutable report.

## Failures, retention and boundaries

Earlier CI runs were **FAIL/STOP**, not PASS, and are retained as history. Original STOP host ZIPs from runs `44385138387`, `44385275850`, `44385405485`, `44385667313` and `44385962461` are byte-identically nested within the private evidence ZIP, alongside the final host and ARM originals. Additional intermediate failures remain in Actions history and are not claimed to be archived as raw ZIPs.

The archive's `MANIFEST.json` records per-file SHA-256 checksums and the nested originals. ZIP CRC passed. The owner-held private Library folder is `/LXR Firmware Lab/`; private files are not published by this GitHub report. The evidence ZIP is not an installation package.

**Unmeasured:** physical device MIDI I/O, stuck-note behavior, CLP audio continuity when changing the selected FX editor, ARM CPU/WCET, audio IRQ/DMA deadlines, runtime stack high-water and long-run stability. Source-extracted host tests cannot substitute for physical testing.

**Explicitly deferred:** changing Delay from freeze/resume to background tail decay (owner prefers current low-cost behavior), and separating an editor-only FX type change from `Distortion_resetClip`. The latter could affect accepted CLP playback continuity and needs a distinct owner approval plus R18 comparison.

**Stable protection:** R22 TEST03 is unchanged at `0e37942250f79306627702061d851731bdfdbbc8`; accepted R18 CLP remains unchanged. No new CI/build/test was run for this documentation-only closeout. No merge, force-push, flash, or release was performed.

**Private KB update:** The owner separately authorized a versioned private KnowledgeBase update. KB ZIP retention/identity must be verified independently of this public documentation branch; the public repository does not store private KB text.
