# R23 — Independent FX parameter-bank core: verified experimental ARM build

**Date:** 2026-11-18 (Europe/Vienna)  
**Disposition:** **HOST PASS / ARM RELEASE BUILD PASS / EXPERIMENTAL / NOT FOR FLASHING**.  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Owner authorization:** implement the first isolated R23 per-effect parameter-bank core, verify it by host tests and ARM CI. The owner explicitly waived backward compatibility with older projects as a design requirement for later kit storage. This **does not** authorize modifying storage now, deleting historical evidence or overwriting existing user projects.  
**Base source (unchanged `release/r23`):** `238389475359f11bf317b446e668e0c82333c231`.  
**Final experimental branch:** `experiment/r23-fx-parameter-banks-core-2026-11-18`.  
**Final exact build/test source:** **`dc9538e0a858c9761c2590d411deba90e8fc64cf`**.  
**Protected stable R22:** `0e37942250f79306627702061d851731bdfdbbc8`; protected `main`: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; accepted R18 sonic reference unchanged.

## Changes and precise scope

Production changes are limited to `Core/Inc/Soundengine/FxBus.h` and `Core/Src/Soundengine/FxBus.c`. The two additional new files are `tests/r23_fx_banks/run.py` and `.github/workflows/r23-fx-parameter-banks-core.yml`. No changes to the four DSP effect implementations, mixer, accepted CLP feedback/sound, MIDI message definitions, ParameterArray mappings, front-panel menu, Kit/Project storage, firmware version display or bootloader.

The `FxBus` control structure now holds five indexed P1–P4 banks (OFF placeholder plus one for **Distortion/CLP, Ringmod, Compressor, Delay**) with `parameterBankType`. The existing `fxBus.parameter[4]` remains an **editor view**, not the authoritative per-effect storage. On selection change, `FxBus_updateParameter()` saves the old bank, restores the selected bank to the view and applies its parameters to that effect. The existing `FxBus_setParameter()` writes the selected bank and applies it. New `FxBus_setBankParameter(type,index,value)` and `FxBus_getBankParameter(type,index)` permit effect-specific access without changing the selected FX page. Indices are checked before access. Existing direct ParameterArray writes are captured on `FxBus_updateParameter()`. All four original processor-specific setter mappings are preserved.

**This is the first core stage only.** Independent per-FX Dry/Wet and physical output routing are **NOT IMPLEMENTED**. Existing `fxBus.dryWet` and `fxBus.audioRouting` continue to control all simultaneously dispatched processors; this path was deliberately left unchanged to avoid prematurely altering audio behavior. MIDI/menu/preset and per-kit persistence integration is **NOT DONE**: the banks are runtime-only and are reset at `FxBus_init()`. In particular, switching or reloading kits cannot yet be claimed to restore all independent FX banks; old or new kit files must not be assumed compatible with this experiment.

## Validation, source identity and preserved first STOP

[Final GitHub Actions run 44327791904](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44327791904) checked out **exact source SHA `dc9538e0a858c9761c2590d411deba90e8fc64cf`** and finished **SUCCESS** in both jobs:
- Host job 134883654860: exact production function bodies extracted and compiled with GCC, **208,950 assertions covering 4,096 deterministic bank selections** in each AddressSanitizer and UndefinedBehaviorSanitizer build, **PASS both**. This includes selecting FX types, changing the currently selected bank, writing an inactive effect, direct legacy ParameterArray-style edits, invalid indices and cross-bank state isolation. This does not simulate real device parameter event timing or use real effect DSP as the bank-test oracle. Production-function extraction SHA-256: `3aa8b07dcd451a6690d4e7829b271c29e2fc733896725df727639b3cf45aa146`.
- The inherited **280,032** R23 shared-FX routing/mask cases, R18 Delay mono, Compressor mono, 135 CLP reference comparisons, USB-MIDI host and R21/R22 Voice/Drum transfer, FatFs storage/transaction regressions all **PASS** in the same host job.
- ARM job 134883654470: GNU Arm 14.3.rel1, CMake Release; full compile, ELF/MAP link and image checksum capture **PASS**; six existing, nonblocking `mixer.c` warnings remain (five enum comparisons, one misleading indentation) and were not modified.
- The first [CI run 44327220562](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44327220562) **STOPPED BEFORE TEST/BUILD** at `git diff --check` because the source-edit insertion had reused some CRLF line endings as added lines. Follow-up commit `dc9538e0...` normalized **only the affected edited regions** and strengthened CI to compare whitespace against the entire pinned R23 source. No history rewrite; the STOP is preserved.
- Independently, the extracted original generated C host harness was recompiled from the retrieved CI artifact using GCC with the same UBSan and ASan flags, then rerun locally: both again returned **0 / PASS 208,950**. This is corroborating host evidence, not hardware validation.

## Static matched release-build resources

| Linker/static quantity | R23 base | Parameter-bank core | Delta |
|---|---:|---:|---:|
| FLASH `.text` | 161,472 B | **161,728 B** | **+256 B** |
| FLASH `.rodata` | 83,272 B | 83,272 B | 0 |
| ITCM used | 13,968 B | **13,968 B** | **0** |
| ITCM free of 16,384 B | 2,416 B | **2,416 B** | 0 |
| DTCM occupied linker span | 109,216 B | **109,300 B** | **+84 B** |
| DTCM nominal free before stack | 21,856 B | **21,772 B** | -84 B |
| General SRAM occupied linker span | 275,164 B | **275,160 B** | -4 B |

The 84-byte DTCM growth is consistent with four 4-float per-effect banks plus an OFF placeholder and selection state/alignment. These are static linker figures, not CPU percentages, safe stack margins, or verified ISR deadlines.

### Binary and source evidence

- CI-created `LXR_V2_V1.7.img`: **297,360 B**, reported SHA-256 `9950655c5b37fdc8687f0f47899eba03bf616306093894294e164ef7385ee790`. The IMG is **NOT** in the downloadable artifact and its bytes have **not** been independently rehashed outside CI. **Do not flash.**
- ELF: **403,128 B**, SHA-256 **`1628e12343d7f9ef874ef81a43e9b8f5101a4a2524151e8efe6bd515e2a675fc`**, verified against original retained ELF bytes.
- MAP: **553,633 B**, SHA-256 **`88300ba7b7f16e4be926e151aa34197576087d3fa2041670de15940ff636feff`**, verified against original retained MAP bytes.
- [ARM artifact 14437058545](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44327791904/artifacts/14437058545): ZIP 306,377 B, SHA-256 **`75e4b1efc7a3c22ddf8f521a7a4f8aa94e2032b6be0d71e178bc9e059698e0cd`**, nested ZIP CRC PASS.
- [HOST artifact 14436954489](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44327791904/artifacts/14436954489): ZIP 28,813 B, SHA-256 **`63eb3f6222eb532569e2c3137c5553514ce1200905b819718f922199f190ff52`**, nested ZIP CRC PASS.
- Original CI artifacts expire on **2026-12-18**. The two original archives have been separately retained and independently SHA-256/CRC-verified. The exact CI build source, host harness source and available log files are archived. No flashable IMG was delivered.

## Limitations, risks and next decision

**Core-level bank isolation is verified, not full user integration.** The current UI has not yet been changed to display different P1–P4 banks with independent Dry/Wet/output. Kit loading, temporary/autosave, project transactions, morph behavior, external MIDI CC/NRPN routing and automation targets are not connected to persistent independent FX banks. The legacy shared Kit serialization still exists, deliberately unchanged; until a separately approved storage redesign, a Kit switch may leave other FX banks from the previous Kit. No new file format or backward-compatibility migration was implemented.

Owner-approved future design constraint: backward compatibility with old project formats is **optional, not required** when it would complicate the new architecture. That does **not** authorize destructive in-place conversion or deletion. A new exclusive, explicit versioned file format, safe rejection of unsupported versions and backup/rollback handling should be designed and authorized separately.

No actual LXR-02 testing was possible because the unit is with its manufacturer for DATA knob assessment. **CPU/WCET, IRQ/DMA deadlines, runtime stack high-water, end-to-end real-device audio and long-term reliability remain UNMEASURED.** The prior four-real-DSP host test on the unchanged R23 source remains background evidence, **not a sound acceptance test of this new bank branch**. The CLP processor code and musically accepted R18 sound reference remain protected.

**Recommendation:** Keep this branch experimental; authorize a separate per-effect Dry/Wet/output integration and menu/CC implementation only after deciding the specific editor selection semantics, then independently design the new Kit persistence contract. No merge to `release/r23`, `main`, firmware release, flashing or hardware tests were performed.
