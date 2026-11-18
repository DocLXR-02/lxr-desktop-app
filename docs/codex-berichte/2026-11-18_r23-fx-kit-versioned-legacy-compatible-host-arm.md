# R23 — Versioned, backward-compatible FX Kit extension: final experimental ARM build report

**Report date:** 2026-11-18 (Europe/Vienna)  
**Repository:** \`DocLXR-02/lxr-desktop-app\`  
**Status:** **HOST PASS / ARM RELEASE BUILD PASS / EXPERIMENTAL / NOT FOR FLASHING**  
**Owner approval:** Extend the existing Kit format with the four independent FX banks, retain legacy project compatibility, use isolated experimental branch, host regression and safety/failure tests, and a validation-only ARM build. No physical tests, release, sound change or merge were authorized.

## Exact identities and protection

| Role | Immutable commit SHA |
| --- | --- |
| Approved experimental source parent (independent FX controls) | \`119f35759ac1c1b218ae94f40fc77ac61a410ceb\` |
| **Final verified experimental source and ARM build** | **\`e6473c707087c0d1d7ebeba260d18bfae61ec901\`** |
| Test branch | \`experiment/r23-fx-kit-extension-2026-11-18\` |
| Selected R23 baseline (unchanged) | \`238389475359f11bf317b446e668e0c82333c231\` |
| Owner-accepted R22 TEST03 (unchanged) | \`0e37942250f79306627702061d851731bdfdbbc8\` |
| Protected \`main\` (unchanged) | \`a43cd4fcda23ec3d577dc425261c3eb28bd5abff\` |
| Documentation lineage | \`672891c23809e8a2014457f564c16732f970e4f9\` |

No production sound effect algorithm files, accepted R18 CLP sonic behavior, bootloader, stable references, \`release/r23\` or hardware were modified. No production candidate was merged or flashed.

## New Kit payload contract

Preserve the **existing eight-byte Kit name and the existing \`END_OF_SOUND_PARAMETERS\`-byte legacy parameter prefix** unchanged, then append exactly 32 bytes to a new named \`.snd\` file:

- **4 bytes:** ASCII magic \`LXFX\`.
- **1 byte:** version \`1\`.
- **1 byte:** payload length \`24\`.
- **24 bytes:** four FX processor records in order Distortion/CLP, Ringmod, Compressor, Delay. Each record stores P1, P2, P3, P4 (0–127), independent Dry/Wet (0–100), and one existing six-choice output routing value (0–5). Total **four × six bytes**.
- **2 bytes:** CRC16-CCITT checksum over preceding 30 bytes, low byte first.

Do **not** hardcode a legacy prefix length such as 228: the exact \`END_OF_SOUND_PARAMETERS\` enum defines it. An early draft incorrectly assumed that length and was stopped by ARM compilation. The final code derives the length symbolically and passed ARM CI.

**Legacy read:** Existing named Kit with no extension retains the original parameter bytes and initializes only the formerly selected FX bank from the legacy P1–P4, Wet and Output; all other banks are zero-initialized. The original independent effect subtype values (distortion algorithm, ring oscillator waveform, compressor ratio, delay mode) remain in existing Kit fields and are not duplicated in this extension. A truly short legacy tail can still be zero-filled by the named-Kit reader. Full projects with standard legacy-sized named Kit files are accepted by the strict transaction validator. The new exact extended payload is accepted with CRC and field-range verification. Invalid extension length, version, CRC or out-of-range FX values are rejected, not silently interpreted as a legacy Kit.

**RAM/SD integration:** A separate 24-byte FX-bank snapshot for **each of 64 Kits** is retained in the preset manager. The active Kit is exported to this storage during capture/saving and appropriate Kit switches, then restored through \`FxBus_importKitBanks\` on loading. Both named \`.snd\` and temporary \`/tmp/tmp.snd\` formats gain the extension (the temp Kit starts with the original single slot byte instead of an eight-byte name). Autosave, the pre-existing per-byte write/fault boundaries, and checked tail write use the existing shared-FatFs error latch. Project validation reads and CRC-verifies any extension in the staging folder **before** publication/rename. The disk-space estimator includes the longer Kit files. There is no new file extension or parallel project hierarchy.

The existing MIDI/menu page and original DSP processor bodies remain inherited from the tested independent-controls parent; changing FX banks can still alter audible processing on future hardware, **not auditioned here**. Distinct MIDI IDs for each separate FX, automation targets, and Morph semantics for the additional bank fields are **not newly implemented**.

## Final source/test diff

Twelve changed/added paths, compared with the exact parent:

1. New \`Core/Inc/Preset/FxKitFormat.h\` — versioned 32-byte codec, CRC and bounds checking.
2. \`Core/Inc/Soundengine/FxBus.h\` and \`Core/Src/Soundengine/FxBus.c\` — FX bank snapshot export/import; no DSP body changes.
3. \`Core/Src/Preset/presetManager.c\` — per-Kit FX RAM, legacy conversion, named Kit and tmp Kit save/load, Kit RAM switches, project-load state guard.
4. \`Core/Src/Preset/projectStorage.inc\` — legacy/new strict validation and CRC, disk-space calculation.
5. \`tests/project_storage/host.c\` and \`tests/project_storage/run.py\` — original FatFs fixture adaptation, legacy/new/corrupt-tail tests, tested storage helper extraction.
6. \`tests/project_transaction/whole.c\` — original whole FatFs power cut/transaction coverage plus full 64-Kit legacy project load and staged FX-tail corruption.
7. \`tests/r22_drum_copy/navigation_cases.c\` — retain original Kit prefix equality oracle and validate appended CRC.
8. New \`tests/r23_fx_kit_format/codec.c\` and \`tests/r23_fx_kit_format/run.py\` — portable codec, 4,096 deterministic valid round trips and malformed-block probes under ASan/UBSan.
9. New \`.github/workflows/r23-fx-kit-extension.yml\` — unchanged inherited host/ARM validation plus codec and legacy project tests; evidence excludes installable IMG.

## Actual executed final CI (not inherited)

[**Final GitHub Actions run 44350715125**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44350715125), source SHA **\`e6473c707087c0d1d7ebeba260d18bfae61ec901\`**, both jobs **SUCCESS**:

- **Host job 134965259763:** 4,096 FX Kit codec roundtrip scenarios with invalid-length/version/CRC/parameter probes, ASan/UBSan PASS; inherited bank selection tests, 280,032 shared FX selector cases, R18 CLP/Delay/Compressor, USB MIDI, R21 voice copy and R22 Drum copy regressions PASS.
- Production-body and real-FatFs tests: named legacy Kit → new Kit extension, corrupted appended block rejected without RAM mutation or leaking the shared file handle; temporary Kit invalid CRC rejected; project containment and 194-file project layout preserved.
- Whole real FatFs FAT16+FAT32 simulation: complete initial and repeated project save/load, 20 replacement cycles per media type, **120 fault/retry cases** including staged FX CRC corruption before publish, **50 logical cut points**, **16 sector-cut/rename cases**; no asserted false success or unsafe replay; both old and new complete projects retained on interrupted transactions in covered conditions.
- **Legacy project compatibility:** two newly added real FatFs host cases — **64 unextended named Kit files per complete project**, first FAT16 then FAT32, strict project validation, project load, former selected FX bank reconstruction for every slot — **PASS**.
- **ARM Release job 134965259105:** pinned GNU Arm 14.3.rel1 CMake Release firmware compilation and link **SUCCESS**. Existing six non-blocking mixer warnings (five enum-compare, one misleading-indentation) remain. A CI IMG was generated only for compilation/checksum identity; **NO IMG DELIVERED**.

The actual host/Safety tests do **not** emulate physical card timing, panel MIDI interactions or on-device live audio. Audio CPU/WCET, IRQ/DMA deadline, runtime stack high-water and long-term reliability remain **UNMEASURED**; the LXR-02 device is with its manufacturer.

### Static matched ARM resource evidence

| Section / region | Previous independent FX controls | Final FX Kit extension | Delta |
| --- | ---: | ---: | ---: |
| FLASH \`.text\` | 162,808 B | **167,528 B** | **+4,720 B** |
| FLASH \`.rodata\` | 83,272 B | 83,272 B | 0 |
| ITCM occupied | 13,968 B | **13,968 B** | 0 |
| ITCM nominal free | 2,416 B | **2,416 B** | 0 |
| DTCM occupied | 109,324 B | **109,324 B** | 0 |
| General SRAM occupied | 275,160 B | **276,952 B** | **+1,792 B** |

DTCM nominal free before stack: 21,748 B. General SRAM nominal free: 116,264 B. These **static** figures do not establish free runtime stack, CPU cycle reserves, interrupt timing or DSP sound parity on hardware.

### Build/artifact identity and retention

- CI-generated image \`LXR_V2_V1.7.img\`: **303,160 B**, CI **reported** SHA-256 \`00754661107b2b06a40e72169e0c2f46f3d53eff82619ebfd49efe944b037e4e\`. The **IMG bytes are not archived or independently rehashed** outside CI and are not made available to flash.
- Original ARM ELF: **412,060 B**, independently verified SHA-256 \`abd084e184a4cf601625ec7bcf0395ece0ec8c5273348473556302ea3568a39e\`.
- Original MAP: **555,508 B**, independently verified SHA-256 \`42a0c7afe0a16657444b936218b7194ea44b69baa148786cb1b98389418d107f\`.
- [Final HOST artifact 14446904515](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44350715125/artifacts/14446904515): **42,223 B**, independently downloaded, SHA-256 \`34104f7add0d085549e99ef66882624f88e3e99d0bb39c36e870757821d48667\`, ZIP CRC PASS.
- [Final ARM artifact 14448441679](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44350715125/artifacts/14448441679): **310,673 B**, independently downloaded, SHA-256 \`93fd43b52f50c507f704182971dbcf0699a5ce00d2f1f51743c7f1104305de50\`, ZIP CRC PASS.
- GitHub original artifacts expire **2026-12-18**. Their bytes, original Zip contents and decisive historical STOP/success archives are also independently retained in the owner evidence ZIP. A local archive alone is not permanent public preservation.

### Preserved CI STOP/FAIL history (not erased by final PASS)

- [44348159687](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44348159687): whitespace/CRLF gate STOP before compilation.
- [44348328424](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44348328424): portable CRC signedness \`-Werror\` STOP.
- [44348465979](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44348465979): **incorrect hardcoded legacy enum length** and missing forward declaration in first draft; fixed to symbolic \`END_OF_SOUND_PARAMETERS\`.
- [44348906921](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44348906921): inherited R22 test expected old exact Kit file length; fixture now checks prefix plus new CRC block.
- [44349223533](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44349223533): inherited project fixture expected old aggregate file size; updated for 64 × 32-byte tails.
- [44349524922](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44349524922): regrouped Kit prefix writes changed fault injection boundaries; restored original per-byte checked writer and appended one checked tail.
- [44349839008](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44349839008): PASS inherited suite.
- [44350354807](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44350354807): PASS additional 64-Kit FAT16/FAT32 legacy migration test.
- [44350715125](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44350715125): final PASS after staged FX CRC tamper test.

These are separate genuine CI runs; no failed state was mislabeled as PASS. Final test source differs from previous pass source by explicit added fault test only.

## Open issues / next gate

- **Unmeasured physical effects:** no real audio listening/CLP acceptance, CPU/WCET/IRQ/DMA/stack/SD-card power-cut hardware measurement. No flash/release.
- **UX/MIDI:** the inherited selected-FX menu and CC2 bindings were host-checked before this task, but the new Kit save/load panel interactions were not device-tested. Per-effect unique CC/NRPN, automation destination assignments and Morph rules for the appended bank fields are not implemented.
- **Compatibility direction:** older \`.snd\` and older standard projects were proven readable by **new experimental firmware in host simulation**. Do **not** assume older firmware can open *new* projects because strict project file sizes differ. Back up old SD projects before any future device test.
- **Design choice:** legacy Kit migration fills only its formerly selected effect's bank; other effect banks start at zero. More elaborate FX defaults or simultaneous-effect initialization remain an owner decision.
- **Validation scope:** the new state-serialization code is compiled and tested with real FatFs host fixtures but is **not** independently retested in a full four-real-DSP combined audio graph for this precise commit. Hardware/audio acceptance remains open.

**Final task disposition:** versioned append-only Kit extension complete on isolated experiment branch; true older project compatibility verified in hosted FAT16/FAT32 tests; host and ARM CI PASS; NOT FOR FLASHING. No merge or protected ref change.
