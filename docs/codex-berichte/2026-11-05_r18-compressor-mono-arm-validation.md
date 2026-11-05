# R18 Compressor Mono — ARM Release Validation Build Report

**Date:** 2026-11-05 (Europe/Vienna).  
**Status:** **PASS — isolated ARM build and independently verified archive. NOT A RELEASE OR DEVICE INSTALLATION.**

## Authorization, decision and scope

The owner expressly approved signed 16-bit saturation for over-range mono Compressor output as the desired policy, followed by an isolated ARM validation build. This approves the *conversion policy* and the *build*, **not** physical listening acceptance, merging, firmware installation, a release or any broader DSP change. The accepted FX-CLP R18 sound reference is frozen; the Compressor correction deliberately defines behavior in the formerly undefined overflow domain.

This task executed directly in CHAT using the already successful host candidate and GitHub Actions CI. No Codex or Work task was needed and no agent quota was consumed knowingly. Live account quota information was not available. All production source changes were completed and source-reviewed in the preceding, separately recorded Compressor host task; this ARM task added only validation/test files.

| Identity | Exact value |
|---|---|
| Repository | DocLXR-02/lxr-desktop-app |
| Accepted original R18 source | **41b364067bce0d8cde624c184933d9cb88e2725b** |
| Original R18 IMG | LXR_V2_R18.img, **287,944 bytes**, SHA-256 **c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1** |
| Corrected, host-tested Compressor source | **ff8f7ae5a7217f65928318f23e8c69b1e6a22485** |
| Source branch | fix/r18-compressor-mono-saturation (unchanged) |
| ARM validation branch | validation/r18-compressor-mono-arm-2026-11-05 |
| **Actual ARM-built commit** | **3b86e2bcd87fd48eb1de9f9e66650d3583650448**; direct child of the host-tested candidate |
| Added files beyond the host-tested candidate | .github/workflows/r18-compressor-mono-arm-validation.yml; tests/r18_compressor_mono/validate_arm.py |
| Final original/candidate Compressor.c blobs | 9e5f34bcecae43a9696cca30658c34a668c8f679 / **7bb84666575c0a35e93e10563aa5b5c559432030** |
| CI | [SUCCESS run 42067914809](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42067914809), attempt 1, job **127387161317** |
| CI evidence artifact | **13434228353**, r18-compressor-mono-arm-validation-evidence, retention 30 days; expires **2026-12-05 16:47:24 UTC** |
| Report-only documentation base | **3cdca52d926c220c3bbcf2c5cc32bc05a5681452** on docs/lab-project-state |

The code correction itself changes precisely **one line** in Core/Src/Soundengine/DSPAudio/FX/Compressor.c, replacing the unchecked mono float-to-int16 conversion with the pre-existing stereo-style __SSAT(...,16) saturation policy. The entire accepted CLP implementation, mixer, FX bus, Delay, USB transport, boot code and all historical protected branches remain unchanged. The original R18 image was neither overwritten nor reissued.

## Reused evidence and executed build

The prior [Compressor host report](https://github.com/DocLXR-02/lxr-desktop-app/blob/57a90213502c5e827f87d49edec5b601700c3de6/docs/codex-berichte/2026-11-05_r18-compressor-mono-saturation-host.md) had already reproduced positive/negative original mono float-cast-overflow faults, verified candidate signed saturation, and demonstrated **144 exact normal-domain output comparisons**. The prior Delay and USB-MIDI ARM reports and original R18 archived CI evidence supplied the reproducibility format and matched build baseline. This task reused those results and ran the necessary focused checks for the **one** newly authorized ARM build; it did not re-run historical unrelated audits.

**Environment:** GitHub Actions ubuntu-24.04; Arm GNU Toolchain 14.3.Rel1, arm-none-eabi-gcc **14.3.1 20250623**; toolchain archive SHA-256 **8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd**. CMake Release preset and the original R18 compile_commands.json were matched **byte-for-byte** in the independent evidence comparison. No bootloader image was configured or built.

**Actually executed, all PASS:**

1. Pinned parent and source-diff preflight. Confirmed the validation commit is a direct child of the exact host-tested Compressor SHA, only two validation/test paths were added, candidate ancestry is the accepted R18, the Compressor production Git blob matches its pin, and CLP, Delay, mixer, FX bus, USB CDC and main boot files match accepted R18 byte-for-byte.
2. Source-extracted host sanitizer fixtures: **10/10 correctly classified process cases**, including expected original positive and negative overflow failures, passing corrected mono saturation and passing original/candidate stereo saturation. **144 in-range normal-domain channel/ratio/make-up/dry-wet configurations** remained byte-identical.
3. R18 CLP and general host regressions: diode state, idle/reopen, FX bus, mixer bounds, **75,000 differential mixer comparisons, 135 core snapshots, 16,384 MAX selector cases and 4,096 ramp endpoints**, plus BUG001, kit loading, storage and Euclidean tests — PASS.
4. ARM toolchain archive digest verification — PASS.
5. **One ARM Release build**: cmake --preset release and cmake --build --preset release --parallel 2 — PASS, 123 Ninja build steps, no compiler warning/error messages.
6. Target artifact validation: validated LXRV2IMG header, internal checksum, payload size, R18 startup marker, stack/reset vectors, ARM ELF identity, **four load segments** whose bytes match the IMG, and all region capacity checks — PASS.
7. Evidence archived with ELF, MAP, IMG, exact SOURCE.zip, compile_commands.json, build logs, JSON sections, compiler/toolchain identity and two SHA-256 manifests. The CI evidence ZIP was downloaded and inspected independently after the GitHub job completed — PASS.

The final run completed in its first CI attempt with all job steps successful. No failed ARM attempt or subsequent rebuild took place.

## Matched memory/resource comparison

All quantities are **bytes**, from the same toolchain/configuration against the archived original R18 Release build, not inferred from an unrelated reference.

| Metric | Original R18 | Compressor-only candidate | Change |
|---|---:|---:|---:|
| Ordinary FLASH .text | 151,784 | 151,784 | **0** |
| ITCM executable (.itcm_text) | 15,208 | 15,208 | **0** |
| DTCM sections (.dtcm + .dtcmz) | 107,420 | 107,420 | **0** |
| General SRAM (.data + .bss + ._user_heap) | 274,644 | 274,644 | **0** |
| FLASH application load payload | 287,928 | 287,928 | **0** |
| IMG including 16-byte container header | 287,944 | 287,944 | **0** |

All **18** candidate linker section sizes equal the independently archived original R18 section sizes. The original historical region-capacity summary also lists +4 bytes in DTCM and general SRAM compared with the section-only accounting above; both conventions correctly yield **zero delta** for this candidate. Remaining nominal margins: **ITCM 1,176 B**, **DTCM 23,652 B** under section accounting (before actual stack use), and **general SRAM 118,572 B** under section accounting. The nominal 16-KiB ITCM ceiling is unchanged.

**Same size is not binary identity:** The candidate IMG SHA-256 differs from the original R18 image. Its ELF also differs from the original by 439 byte positions despite the identical total size; the MAP bytes remain identical. This is expected for a changed code sequence of equal footprint, and does not prove equal CPU timing or sound.

## Independent evidence checks and checksums

The downloaded CI ZIP and reference R18 archive were inspected independently of the build scripts. The results were not accepted only on GitHub's green job indicator.

| Artifact | Size | SHA-256 |
|---|---:|---|
| Full downloaded Compressor ARM evidence ZIP | **2,180,983 B** | **28cf00751e6785684a1654bd9c2990073b8e6ac6aa3d19bb196206b82f0a69c5** |
| Validation-only Compressor IMG | **287,944 B** | **8d8bd64dadfad17d19f684a4bd2b567de3b3d19f437195abce1b82f4c02d25a6** |
| ARM ELF | **391,712 B** | **607313ac830f1f9b35a65b74632b008149bafca51d3f7019e664698e907ad877** |
| ARM MAP | **541,107 B** | **2d2e6b0fe6235e122bcfa517c84526f945e43371cab5f09230ea76f4a63c7448** |
| SOURCE.zip | Exact CI source archive | **c3a4829413c0b4cb5da9a7580fdcfd8b2063b558228a8da42b38f6dc7dc57945** |

- Outer evidence ZIP: **33 entries, CRC PASS**.
- Primary file manifest VERIFIED_HASHES.txt: **5/5 PASS**.
- CI-record manifest RECORD_HASHES.txt: **32/32 PASS**.
- Original R18 and candidate SOURCE.zip: internal ZIP CRC **PASS**, **398 common files unchanged**; only original Compressor.c changed, by exactly the approved one-line replacement; four added test/workflow files, no removed original source.
- Original R18 versus candidate compile_commands.json: **byte-for-byte identical**.
- Compiler archive SHA-256 and compiler versions: **matched**.
- Independently parsed ELF: **four loadable FLASH segments**, each byte-identical to the corresponding IMG location. Header checksum, firmware payload size and boot marker PASS.
- Original R18 and candidate all **18 section sizes** matched; only executable bytes, not static sizes, differ.

The evidence ZIP is retained at the [GitHub CI artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42067914809/artifacts/13434228353), subject to its recorded 30-day expiration. A copy is retained in the owner's private Library at /LXR Firmware Lab/LXR_R18_Compressor_Mono_ARM_Validation_Evidence_2026-11-05.zip. Neither artifact is an authorized firmware installation package. The build's generic updater instructions in its normal compiler log do not confer owner approval to flash.

## Approval limits, sound and outstanding risks

**OWNER DECISION:** The mono signed 16-bit saturation policy, which aligns mono overflow with existing stereo behavior, was explicitly approved. **OWNER HARDWARE LISTENING:** Not performed or accepted. Over-range output may differ audibly from original undefined behavior; no full-domain sound-identity claim is made.

**NOT RUN / NOT APPROVED:** Installation/flashing, physical hardware playback, acoustic comparisons, integrated Delay+USB+Compressor firmware, firmware delivery/release, merge into accepted R18 or main, bootloader modifications, physical USB/MIDI stress or extended device validation.

**UNMEASURED:** Actual Cortex-M7 audio CPU/WCET, IRQ/DMA deadlines/underruns, stack high-water, target audio stability, thermals and long-term reliability. Compiler/static size equivalence is not proof of runtime cycle equivalence or correctness on a physical device.

**Next decision, requiring separate approval:** Prepare a **new, explicitly named integration branch based on the immutable accepted R18 SHA**, bring in only the three individually tested Delay, USB-MIDI and Compressor production fixes, independently verify the combined source/diff and repeat focused host/ARM checks with matched resource and image hashes. Preserve original R18 and the three isolated candidates. There is no authorization in this report to merge, flash, distribute an installation image or conduct physical testing.

## Immutable references

- [Exact Compressor ARM build source commit](https://github.com/DocLXR-02/lxr-desktop-app/tree/3b86e2bcd87fd48eb1de9f9e66650d3583650448)
- [Compressor mono ARM validation workflow](https://github.com/DocLXR-02/lxr-desktop-app/blob/3b86e2bcd87fd48eb1de9f9e66650d3583650448/.github/workflows/r18-compressor-mono-arm-validation.yml)
- [Compressor ARM independent validator](https://github.com/DocLXR-02/lxr-desktop-app/blob/3b86e2bcd87fd48eb1de9f9e66650d3583650448/tests/r18_compressor_mono/validate_arm.py)
- [Successful CI run 42067914809](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42067914809)
- [Earlier Compressor host report](https://github.com/DocLXR-02/lxr-desktop-app/blob/57a90213502c5e827f87d49edec5b601700c3de6/docs/codex-berichte/2026-11-05_r18-compressor-mono-saturation-host.md)
- [Accepted original R18 source](https://github.com/DocLXR-02/lxr-desktop-app/tree/41b364067bce0d8cde624c184933d9cb88e2725b)

**Publication location:** Dedicated report-only branch based on canonical docs; no production or canonical documentation branch was modified.
