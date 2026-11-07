# LXR Firmware Lab — R19 Version-Only Candidate ARM Release Build Report

**Date:** 2026-11-07 (Europe/Vienna)  
**Status:** **PASS — one reproducible R19 candidate ARM Release build, inherited regression suites and independent binary audit. CANDIDATE ONLY: not a final release and not independently installed on hardware.**  
**Record type:** One separate verified report for this single delivered firmware candidate build. Reuse of older R18 technical and owner observations is labeled separately.

## Authorization and scope

After the owner reported that transient clicks had disappeared upon reloading both a different project and the originally affected project, the owner explicitly accepted the missing physical USB-MIDI test as **not blocking R19 candidate preparation**, and granted preparation approval. The approved preparation included the firmware startup version marker update, a new isolated ARM Release build, the existing regression suites, static resource and image validation, and a per-build report.

**Authorization excludes:** final release/public release, merge into `main` or accepted R18, flashing/installing this R19 candidate, bootloader changes, unapproved audible/signal-path modifications, new device hardware tests or any assumption that USB-MIDI has passed physical validation. The original musically accepted R18 and the integrated three-fix source remain untouched.

The change is sufficiently limited for **direct CHAT and GitHub CI**; neither Codex nor Work was used. No agent quota or budget was consumed or estimated by this report.

## Exact identities

| Item | Identity |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Accepted, protected musical R18 source | `41b364067bce0d8cde624c184933d9cb88e2725b` |
| Previously owner-installed three-fix TEST source | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` |
| **New candidate source branch** | `candidate/r19-integrated-three-fix-2026-11-07` |
| **Exact candidate source HEAD** | **`638b6eaae5b1624637d20cbc11a4a3e0175edca2`** |
| Intermediate boot-string commit | `454398997931317cbe26b2f58651ec6c6c0e1d08` |
| Validation-script commit | `03fc2b554e175ff6bb009ed3de48817247fedc3a` |
| CI workflow commit (final candidate source) | `638b6eaae5b1624637d20cbc11a4a3e0175edca2` |
| **Successful GitHub Actions run** | [**42407160858**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858), attempt 1, job `128507226577`, **completed SUCCESS** |
| ARM compiler | Arm GNU Toolchain `14.3.Rel1`, GCC `14.3.1 20250623`; exact toolchain archive SHA-256 `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd` |
| Build mode | CMake `--preset release`; `cmake --build --preset release --parallel 2`, Ubuntu 24.04 |
| New boot line | `LXR LAB R19` |
| Firmware IMG in archival and review ZIPs | **`LXR_V2_R19_CANDIDATE.img`**, **288,184 bytes** |
| **Candidate IMG SHA-256** | **`a1d53435ce922c4ef57ce595bf8c180b5225340780b6ad3241b6340af652e57b`** |

## Source scope and direct-parent verification

GitHub comparison of the pinned three-fix integration base against final R19 candidate yields precisely **three changed repository paths**:

1. `Core/Src/main.c`: **one single production source line**, replacing `lcd_string("LXR LAB R18");` with `lcd_string("LXR LAB R19");`; no other source changes.
2. `tests/r19_candidate/validate_arm.py`: new build/image/sections/identity validator, based on and extending the existing integration ARM checks.
3. `.github/workflows/r19-candidate-arm-validation.yml`: new isolated branch-scoped CI workflow reusing the full inherited source-host tests, toolchain and Release build.

CI independently checked that `git merge-base HEAD cdc294c... = cdc294c...`, that the above paths are the exact allowlist, that `Core/Src/main.c` equals the original after just the one line substitution, and that `#define UPDATE_BOOTLOADER 0` is unchanged.

The **five inherited non-boot production source blobs** were reverified unchanged in CI:

| File | Git blob |
|---|---|
| `Core/Src/Soundengine/DSPAudio/FX/Compressor.c` | `7bb84666575c0a35e93e10563aa5b5c559432030` |
| `Core/Src/Soundengine/DSPAudio/FX/Delay.c` | `1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6` |
| `Core/Src/Soundengine/USB/usb_manager.c` | `7f30ed0fcdb433f280b98bb238879cb28834b585` |
| `Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h` | `56bb9165ae5d2bf0134a55a4a95652bc706aef77` |
| `Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c` | `ef33e3e2941db2a8a72705d66d419c29966f4d47` |

No CLP, oscillator, mixer, effect parameter, kit/pattern, routing, codec, bootloader or hardware configuration code was edited.

## Tests actually executed in CI run 42407160858

All independent CI steps **PASS**, without a failed attempt:

| Step | Verified result |
|---|---|
| Exact source/allowlist and inherited blobs | PASS |
| Delay original fault reproductions and candidate normal/ping-pong mono tests | PASS; **144 byte-level comparisons**; stereo/mono-left fixture outputs unchanged |
| USB-MIDI original fault reproductions and corrected TX/RX handling | PASS; BUSY, in-flight ownership, ring wrap, full RX, disconnect and short packets covered by source-extracted host fixtures |
| Compressor original float-to-int16 overflow reproductions | PASS; corrected mono saturation and **144 normal-domain raw-sample parity cases**; stereo fixture paths exercised |
| FX-CLP R18 DSP and bus regressions | PASS; **75,000** independent output mix cases, **135** R14 feedback-state comparisons, MAX control **16,384** cases and all six FX output routing fixtures |
| Other inherited host regressions | PASS; bug001, kit load, storage, Euclidean suites (logs included) |
| Exact matched GNU ARM toolchain | PASS; archive SHA checked |
| Full ARM Release compile and link | PASS |
| Image header/vector/checksum, ELF/image load parity, region capacities, boot string and exact matching baseline section sizes | PASS |
| Source archive, checksums and evidence retention | PASS |

**Limit:** The tests are bounded host simulations plus successful static ARM build, not on-target timing, end-to-end USB-MIDI traffic or hardware sound certification for the new R19 boot-label image.

## Independent artifact audit

After CI completed, the exact official workflow artifact was retrieved and examined independently:

- **CI evidence ZIP:** `LXR_R19_Candidate_ARM_Validation_Evidence_CI.zip`, **2,197,817 bytes**; SHA-256 **`d1d8d30c656a8749a08c0ad63cd5be162878baf82b7877993e0e662bb92ad034`**.
- **GitHub Actions artifact:** ID **`13587584482`**, name `r19-version-only-arm-candidate-evidence`, retention 30 days, expires approximately **2026-12-07T19:38:47Z**. [CI artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858/artifacts/13587584482).
- Evidence ZIP contains **35 entries**; outer ZIP CRC **PASS**; `VERIFIED_HASHES.txt` **5/5 PASS** and `RECORD_HASHES.txt` **34/34 PASS**. Embedded full source ZIP CRC PASS.
- Original three-fix source ZIP versus candidate source ZIP: **exactly one changed common path, `Core/Src/main.c`**; two new infrastructure/test paths; no source deletions. Full main.c is byte-identical except the startup string replacement.
- Matched `compile_commands.json` file is **byte-for-byte identical** to three-fix integrated CI, SHA-256 `c66381f02a90e7ef1b2fb37369849e7e8e4ee4c3b689f4942211ef2a6b1acdf8`.
- New R19 IMG versus integrated pre-R19 TEST IMG: **exactly two differing bytes in the complete 288,184-byte file** — file offset **12** updater checksum `0x1b -> 0x1a`; file offset **152,858** startup marker ASCII `'8' -> '9'`. The entire remaining **288,182 bytes are identical**.
- Updater header signature `LXRV2IMG`, little-endian payload length **288,168**, one-byte checksum, zeroed reserved header bytes, stack and reset vectors, Cortex-M7 ELF identity, **all four load segments byte-for-byte matched** to image offsets and application FLASH address limits: PASS.
- **ARM ELF:** 391,916 bytes; SHA-256 **`dcd3b9a6bc935511ccd8790915b4110dec1f531ce85b886a75b46340d4c7db9a`**.
- **ARM MAP:** 541,577 bytes; SHA-256 **`63e649b2130ae839c7ff166b7cfc45b06c75518d64747ab1539e35dddc29ed95`**. Identical to integrated candidate MAP.

**Owner review ZIP:** `LXR_R19_Candidate_Review_2026-11-07.zip`, **207,428 bytes**, SHA-256 **`7ad488b3ce136415aaf5825e875eccd8e36b9fa6c308a990cc2ab93b63e974ec`**. ZIP CRC PASS, three English-language entries only: R19 candidate IMG, `SHA256SUMS.txt`, and `R19_CANDIDATE_NOT_FOR_FLASHING.txt`; extracted IMG matches its SHA. This is a review/retention package, **not installation permission**.

Both archives were independently checked and saved to the owner's private Library under `/LXR Firmware Lab/`. Full source, map, ELF, compiler metadata, CI tests, validation JSON, and logs remain in the evidence ZIP; a GitHub artifact alone has an expiry and is not durable evidence.

## Matched resource accounting

All bytes. The original accepted R18 base is for historical comparison; the immediate source-matched integration baseline is the correct measure of a version-only change.

| Metric | Accepted R18 | Three-fix integration | R19 candidate | R19 vs integration |
|---|---:|---:|---:|---:|
| FLASH `.text` | 151,784 | 152,024 | 152,024 | **0** |
| ITCM `.itcm_text` | 15,208 | 15,208 | 15,208 | **0** |
| DTCM `.dtcm + .dtcmz` | 107,420 | 107,420 | 107,420 | **0** |
| General SRAM `.data + .bss + ._user_heap` | 274,644 | 274,660 | 274,660 | **0** |
| FLASH full load payload | 287,928 | 288,168 | 288,168 | **0** |
| Complete update IMG | 287,944 | 288,184 | 288,184 | **0** |

Nominal remaining, inferred solely from static section totals: **ITCM 1,176 B**, **DTCM 23,652 B**, general SRAM **118,556 B**, application FLASH capacity nominal remainder **727,640 B**. These are **NOT** live memory-usage measurements. DTCM nominal gap is not actual safe stack reserve; map and section allocations use source-matched definitions.

## Hardware provenance, residual risk and release boundary

**Owner device observations apply to the earlier installed integrated TEST image, not to this R19 build:** owner reported a successful earlier TEST installation, satisfactory CLP sonic character with multiple voices and filter enabled, and normal Delay `Del`/`Pp` with six simultaneous voices in tested output modes. Later, transient clicking that initially appeared across effects and output groups was reported on original manufacturer and lab firmware, then disappeared after loading another project **and after reloading the originally affected project**. No persistent Compressor regression was established; root cause of transient reload-sensitive state remains unknown. This report does not claim a controlled repeat of all Compressor output/gain conditions on the new R19 build.

**Physical USB-MIDI test: NOT RUN/BLOCKED**, because owner lacks suitable equipment. The owner explicitly accepted this as **not a blocker for R19 candidate preparation**, not as a physical USB-MIDI PASS. The bounded ring can still drop new events at capacity, including potentially important Note-Off messages; exactly-once delivery during disconnect is not guaranteed.

**Still UNMEASURED:** Cortex-M7 CPU/WCET and complete audio-block deadlines, IRQ/DMA service timing or underruns, live stack high-water, sustained high-load/thermal stability, and long-term reliability.

**Next decision:** Owner may approve a narrowly scoped installation and ordinary on-device acceptance test of this byte-verified R19 candidate using the existing verified SD backup and R18 recovery procedure. A successful CI build and the two-byte identity demonstration do **not** themselves authorize flashing or final release. Final promotion/publication requires a separate explicit release decision and preserved test evidence. Protected `main`, accepted R18, R4/R11 and the three-fix integration commit stay unchanged.

## Source and evidence links

- [Exact R19 candidate source](https://github.com/DocLXR-02/lxr-desktop-app/tree/638b6eaae5b1624637d20cbc11a4a3e0175edca2).
- [R18 three-fix integration -> R19 candidate diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6...638b6eaae5b1624637d20cbc11a4a3e0175edca2).
- [Verified successful R19 CI run](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858).
- [Historical integrated R18 three-fix ARM build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/8ce74dfb692e08a605d6a4817134f03fb21180ff/docs/codex-berichte/2026-11-05_r18-three-fix-integration-arm-validation.md).
- [Owner revised FX-routing and reload-sensitive clicking chronology](https://github.com/DocLXR-02/lxr-desktop-app/blob/916a4fd272cf01c8e535cf796f1b43b5156df0d9/docs/hardware/2026-11-07_shared-fx-routing-owner-observation-reclassification.md).

**Report publication:** Documentation-only branch `reports/2026-11-07-r19-candidate-arm-build`, created from the pinned canonical docs source `3cdca52d926c220c3bbcf2c5cc32bc05a5681452`, not from a firmware branch. This report does not modify a production source or create a final release.
