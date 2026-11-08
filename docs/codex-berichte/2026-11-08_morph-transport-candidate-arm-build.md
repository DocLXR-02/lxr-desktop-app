# Matched ARM Release build report — Isolated Morph command guard candidate

Date: 2026-11-08, Europe/Vienna.

## Identity and scope

- Repository: `DocLXR-02/lxr-desktop-app`.
- Immutable firmware source SHA: `c433f0ecc68ba66dc885784ad03905c8be18cf39`.
- Matched comparison counterpart: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`.
- GitHub Actions run: https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268, job `matched-arm-build`, conclusion **SUCCESS**.
- Evidence workflow source SHA: `3a4e846c99f14e62a5b56ce8c6e9d22831f26823` on separate branch `ci/morph-transport-arm-compare-2026-11-08`.
- Toolchain: `arm-none-eabi-gcc 14.3.1 20250623`, Arm GNU Toolchain `14.3.Rel1`; Ubuntu 24.04; same single CI run for both source commits.
- CMake `release` preset (`-O3`), firmware application only; no bootloader.
- Explicit authorization: owner approved the next limited technical build comparison; **no flash, hardware test, release, merge, or upstream PR**.
- Identical public upstream source plus exactly 21 inserted production-code lines in `Core/Src/Preset/presetManager.c`; the external MIDI parser is untouched.

## Actually executed commands and checks

- `git worktree add --detach ... c433f0ecc68ba66dc885784ad03905c8be18cf39` from a full-history checkout.
- `cmake --preset release` and `cmake --build --preset release --parallel 2`.
- `arm-none-eabi-size -B` and `arm-none-eabi-size -A`.
- Independent post-build validation: `LXRV2IMG` header magic, payload length, checksum, zero-padding, exact equivalence to objcopy BIN, initial stack alignment and valid Thumb reset vector.
- Build and linker checks **PASS**. Full logs, MAP, native size/section outputs and JSON manifests were downloaded from the CI evidence archive and independently checked for ZIP integrity and agreement with this record.

## Results

| Linker region | Used bytes | Region capacity | Nominal unused bytes |
|---|---:|---:|---:|
| ITCM_RAM | 15008 | 16384 | 1376 |
| DTCM | 107392 | 131072 | 23680 |
| RAM | 271036 | 393216 | 122180 |
| FLASH | 222716 | 1015808 | 793092 |

| Metric | Value |
|---|---:|
| GNU size text | 273860 B |
| GNU size data | 852 B |
| GNU size BSS | 340572 B |
| Complete application IMG | 274736 B |
| Firmware payload | 274720 B |

- Complete IMG SHA-256: `b503f43a22201cc0eac1fdb3a6cb98f75d15e66a849615ab154f3692152a3143`
- ELF SHA-256: `113fe9c5b74506359519adc5970a798369891302a14d0d582d0d0c066efa2cfc`
- MAP SHA-256: `ef502b8f10a0f6b5c506dd67de62779fb85655bb8a278cdad7838dc05126b25b`
- Image header checksum byte: `0x1e`.
- Initial MSP: `0x20080000`; Thumb reset vector: `0x080131e5`.
- Flashable IMG and ELF were deliberately **not retained** in the public evidence archive; hashes were retained. This build is not a firmware delivery.

## Evidence retention

- CI run and job logs: https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268.
- CI evidence ZIP artifact ID `13636764067`; archive SHA-256: `28aacae068518c1c1d637a0c4b4c6b1d413acf575c4c6614bce43c27ccbec3cf`.
- ZIP contents include two independent English build reports, matched comparison, raw configure/build logs, linker MAP texts, size and sections output, and validated JSON manifests.
- GitHub-hosted artifact scheduled expiry: 2026-11-15 22:43:30 UTC. A separately verified ZIP copy was downloaded during the investigation; its future persistence depends on conversation file retention. The core report has been preserved in GitHub on this report-only branch.
- The comparison workflow and its complete Python validator are retained at CI commit `3a4e846c99f14e62a5b56ce8c6e9d22831f26823`.

## Verification boundaries

The user reports that the affected Morph behavior exists on original LXR-02 **v1.71**. The official v1.71 production image was not built from a verified matching source tree: the public GitHub CMake metadata identifies itself as 1.7. These two matched outputs therefore compare **source revisions**, not official 1.71 binary equivalence.

The physical instrument has not been flashed with this isolated patch. The previously observed successful Morph listening test involved a combined laboratory firmware, not this isolated patch. Real-device behavior, filter audio, timing, CPU/WCET, IRQ/DMA deadlines, stack high-water, long-run reliability and audio headroom remain **NOT TESTED/UNMEASURED** in this task. Host source-extracted Morph tests succeeded on separate CI run 42530896384.

AI assistance: source inspection, isolation, test automation and report composition were assisted substantially by OpenAI ChatGPT. An independent manufacturer review remains necessary.
