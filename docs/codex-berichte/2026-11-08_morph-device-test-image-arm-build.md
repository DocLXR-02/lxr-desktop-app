# Isolated Kit Morph — verified ARM build report for owner device testing

Date: 2026-11-08 (Europe/Vienna)
Status: **BUILD AND HOST REGRESSION PASS; PHYSICAL DEVICE NOT YET TESTED.**

## Scope and source provenance

- Repository: `DocLXR-02/lxr-desktop-app`.
- Exact production firmware source: `c433f0ecc68ba66dc885784ad03905c8be18cf39`, branch `pr/bug001-morph-transport-guard`.
- Official upstream source baseline: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`.
- Exact production diff: 21 inserted lines, zero deletions, only `Core/Src/Preset/presetManager.c`.
- Build orchestration-only commit: `4ac76d10b30295a1271d58fe20972dfecb5c1644`, branch `test/morph-transport-device-image-2026-11-08`; this is **not** the production source being built.
- [GitHub Actions run 42532306552](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42532306552), job `candidate-test-build`: **SUCCESS**, attempt 1.
- Toolchain: Arm GNU `14.3.Rel1` (`arm-none-eabi-gcc 14.3.1 20250623`), Ubuntu 24.04, CMake Release (-O3).
- No bootloader configured, updated, or flashed. No source sound algorithm, filter Drive curve, MIDI parser, project representation or accepted CLP edit.

## Actually executed tests, builds and checks

- Native regression runner `python3 tests/morph_transport/run.py`: PASS, including negative control showing the original reserved NRPN dispatch; candidate's 256 Morph values, identical-kit replay, restart/endpoint behavior and immediate kit sweep audio-service count.
- Clean release configure/build: `cmake --preset release`, `cmake --build --preset release --clean-first --parallel 2`: PASS.
- ELF link: PASS, all static linker sections fit.
- Generated `LXR_V2_V1.7.img` application image: valid `LXRV2IMG` header, declared length, checksum, padding, exact payload/`.bin` equality, initial MSP, Thumb reset vector: PASS.
- Generated IMG SHA-256 equals the prior independent matched-build candidate IMG SHA-256 from [CI 42531589268](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268): PASS, bit-identical image.
- GitHub Actions image artifact ID `13637652351`: one IMG file, **no Markdown/ELF/bootloader**, GitHub artifact archive SHA-256 `0dd30837dda1314fedd8c6c64cbad3737a5515be5f59b68f01e4cf7c67f7ae7e`.
- Separate evidence artifact ID `13637632513`: per-build source report, full configure/build/regression logs, linker MAP, section/size output, SHA-256 and JSON manifest; GitHub artifact SHA-256 `2377cac29fd6c35cadb1f231c25336a4838e20d137eac9c085b148188309c199`.
- The two artifacts were downloaded independently into the task runtime, passed ZIP integrity checking, and were compared against the actual embedded image and manifest.

## Verified release-build image

| Property | Value |
|---|---|
| File name | `LXR_V2_V1.7.img` |
| Full IMG size | **274,736 bytes** |
| Raw payload size | **274,720 bytes** |
| Image SHA-256 | `b503f43a22201cc0eac1fdb3a6cb98f75d15e66a849615ab154f3692152a3143` |
| ELF SHA-256 | `113fe9c5b74506359519adc5970a798369891302a14d0d582d0d0c066efa2cfc` |
| Image internal checksum | `0x1e` |
| Initial MSP | `0x20080000` |
| Reset vector | `0x080131e5` |
| GNU size text/data/BSS | 273,860 / 852 / 340,572 bytes |

| Linker region | Used bytes | Capacity bytes | Nominal static remainder |
|---|---:|---:|---:|
| ITCM | 15,008 | 16,384 | 1,376 |
| DTCM | 107,392 | 131,072 | 23,680 |
| RAM | 271,036 | 393,216 | 122,180 |
| FLASH | 222,716 | 1,015,808 | 793,092 |

Matched comparison against the upstream GitHub source, same toolchain: ITCM +8 B; DTCM 0; RAM 0; linker FLASH +24 B; full IMG +32 B. The nominal free linker bytes are **not** measured CPU, audio latency, IRQ/DMA or live stack headroom.

## Experimental test packaging

- Private user-facing candidate ZIP `LXR_Isolated_Morph_Test_Image.zip` contains exactly one file `LXR_V2_V1.7.img`; the contained image was verified to match the SHA above, and ZIP integrity passed.
- Candidate ZIP SHA-256: `da17661f604efd9cd79c6f8d542b2367ccc03186864105b4d2384de0007b757b`.
- Manufacturer-supplied, owner-provided original v1.71 IMG SHA-256: `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576`, 273,544 B; this official image was not built by the laboratory.
- Separate private `LXR_Official_v1.71_Reference.zip` includes only that original IMG. ZIP SHA-256: `29057a66c5a5977245cd7833fd73ca1322af4e4260eedd6117b0c15297e08a64`.
- English test plan, CSV matrix (26 cases x A1/B/A2), logging template and image manifest: `LXR_MORPH_Device_Test_Plan.zip`, SHA-256 `c0791149135a97a6edcf8493e0a8127127f839cb558807569db5d52c9b8d9ba1`. The original archives are retained as conversation download files; public GitHub contains this record, but not the IMG or private SD-card project data.
- GitHub Actions artifacts are temporary and expire 2026-11-15 UTC, but original lab package copies and text evidence were retrieved during this task.

## Limitations, safety and provenance

**The original reported defect existed on the owner's installed official manufacturer firmware v1.71.** The briefly used private support firmware is not required to explain this baseline and is not treated as the defect's origin.

The public upstream source sets build version 1.7 and is **not proven binary or source equivalent** to the official production v1.71 image. A1 vs B is therefore a real installed-firmware comparison with a source-provenance uncertainty, not a formally isolated 21-line binary difference from the official production image.

The owner has not yet installed or physically tested this isolated candidate under this plan. A previous successful hardware observation involved a **combined** laboratory firmware. The present image's physical startup, real audio output, MIDI/USB behavior, long-run reliability and project-storage compatibility are **NOT TESTED**. CPU/WCET, IRQ/DMA deadlines, stack high-water and sustained audio safety are **UNMEASURED**. Avoid external MIDI input and known Euclidean length-16 source risk on this exploratory image.

Manufacturer supports official firmware only and cautions that modified firmware is installed at the owner's risk. Device testing requires a separate FAT32 card, verified complete backup, verified recovery image, low-volume monitoring, and the owner to carry out the normal SD-card update. Do not copy multiple `LXR*.img` files into the card root. No flash, debug-probe operation, public GitHub release, PR or merge has been carried out as part of this task.

## AI assistance and review

The owner identified the physical-device behavior and will execute and assess physical A/B/A listening tests. OpenAI ChatGPT substantially assisted with code analysis, minimal patch isolation, CI automation, artifact integrity review and the test procedure. No claim is made that the owner manually authored or audited every code line, that ChatGPT observed the instrument, or that Erica Synths endorsed or independently accepted this change.
