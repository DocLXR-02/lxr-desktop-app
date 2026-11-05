# R18 Delay Mono — ARM Validation Build Report

**Date:** 2026-11-05 (Europe/Vienna)
**Status:** PASS — authorized ARM Release validation build. NOT an approved installation or release.

## Authorization, immutable identities and scope

The owner approved a targeted ARM validation build of the previously host-verified Delay mono-access correction. The owner did not authorize flashing, hardware testing, release, merging, sound modifications or additional defect fixes.

| Role | Identity |
| --- | --- |
| Repository | DocLXR-02/lxr-desktop-app |
| Protected accepted R18 source | 41b364067bce0d8cde624c184933d9cb88e2725b |
| Candidate branch | fix/r18-delay-mono-safe-access |
| Production fix commit | c0ade6400479ced32260a799a4974eef4ae48b93 |
| Previous host-tested candidate | 35802b4abaf0b080e1d0e5d3b35ea90a212a0178 |
| **Exact ARM-built candidate commit** | **08bc165500e21153eeb28ddbf2fe2d84626a82ca** |
| R18 original Delay Git blob | 64d2f4721347c53208d80257ca607b079d987ff5 |
| Candidate Delay Git blob | 1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6 |
| **Success CI run** | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428668 |
| Success job | 127063510153, attempt 1 |
| Build evidence artifact | 13390187381, 30-day configured retention |
| Historical accepted R18 Release build | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117 |

**Only one production file differs from R18:** Core/Src/Soundengine/DSPAudio/FX/Delay.c. Its normal and ping-pong mono-output paths now read/write the selected output channel through an int16 pointer instead of a potentially misaligned int32 read from a mono-right output. The stereo path, delay feedback/timing/tone, CLP effect, mixer and source controls were not changed. The branch also contains isolated host/build test scripts and workflows. The accepted R18 firmware branch remains unchanged.

## Verified executed tests and ARM build

Toolchain: matched original R18 Arm GNU Toolchain 14.3.Rel1, GCC 14.3.1 (2025-06-23), Release -O3 on ubuntu-24.04. Toolchain archive SHA-256:

8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd

Executed commands on exact candidate CI source:

- python3 tests/r18_delay_mono/run.py build/r18-delay-arm-host
- Existing R18 CLP unit, state, mixer, core-isolation, and R14 MAX-control host checks
- cmake --preset release
- cmake --build --preset release --parallel 2
- python3 tests/r18_delay_mono/validate_arm.py

**Results: PASS.** The original R18 mono-right misaligned load was deliberately reproduced in its expected sanitizer-failure fixture. Both corrected Delay modes passed sanitizer-enabled host tests, with **144 byte-level comparisons**, including bit-identical unaffected R18 mono-left and stereo output over tested conditions. R18 CLP regressions passed: 75,000 differential mixer reference cases, 135 core-state snapshots, 16,384 MAX-route tests and 4,096 ramp-endpoint checks, plus unit/state cases. These are bounded host tests, not physical-device timing or all-input sound equivalence.

The ARM Release compiler and linker succeeded. The validation script verified the application IMG signature, header payload length, internal checksum, Cortex-M7 ELF identification, reset/stack vectors, and exact bytes of all **four ELF load segments** against the IMG, with correct FLASH range and region fit.

**Independent verification:** The downloaded R18 build evidence and candidate CI evidence ZIPs passed ZIP CRC checks. All **5 build hash manifest entries** and **22 record hash entries** matched; R18 and candidate used the same compiler-command JSON bytes and original toolchain hash. Comparing the nested source archives showed **334 other production files byte-identical** to R18, including CLP DSP, mixer, FX bus and main boot code. The source Delay Git blob identities matched the recorded commits.

### Separate CI preflight limitation

A *parallel old host-only workflow*, https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428701, failed during its strict changed-file allowlist preflight: the newly authorized ARM validation workflow/test files were not on that older workflow's allowlist. It ran no host/DSP tests. This was not a firmware build failure or a failed audio assertion. The new ARM workflow had its own correct allowlist and passed all its host and ARM gates. The old guard remains a housekeeping item; a redundant ARM rebuild was not triggered merely to silence it.

## Matched resource ledger

Compared independently with the original R18 build's archived section table, actual ELF and full IMG. All **18 named sections** were checked; only ordinary-flash .text changed.

| Region / metric | Accepted R18 | ARM candidate | Delta |
| --- | ---: | ---: | ---: |
| Ordinary-flash .text | 151,784 B | 151,800 B | **+16 B** |
| ITCM .itcm_text | 15,208 B | 15,208 B | **0 B** |
| DTCM .dtcm + .dtcmz | 107,420 B | 107,420 B | **0 B** |
| General SRAM .data + .bss + ._user_heap | 274,644 B | 274,644 B | **0 B** |
| Complete application FLASH load payload | 287,928 B | 287,944 B | **+16 B** |
| Firmware IMG including header | 287,944 B | 287,960 B | **+16 B** |

Remaining nominal ITCM: **1,176 B**. The project's historical aggregate ledger counts 4 additional bytes in each RAM aggregate versus the section-only method above; **both comparisons show zero RAM growth**. Static free space is not a measured stack margin.

Symbol sizes from independent ELF inspection: Delay_process 1,066 to 1,084 B (+18); Delay_processPingPong 1,642 to 1,646 B (+4). Combined function-size changes are not the same as net .text growth because final code layout/alignment can change. Function byte sizes are NOT CPU cycle measurements.

## Independently checked artifacts and identity hashes

| Build file | Bytes | SHA-256 |
| --- | ---: | --- |
| Candidate validation-only IMG | **287,960** | 3ab7f99a893df91474f862ac4f6158c5d11bed896499937a21c50f563cf39abc |
| Candidate ELF | 391,712 | e8af2353fe5eef26be9364025d2fd384aa04fa62d28f6d3b1afe30f4c2e1b3a3 |
| Candidate MAP | 541,068 | 95c493965dcc93b5a2a4000467cc13923d4b4bce6cd1a38df0f79263fe112c45 |
| Candidate SOURCE.zip | 1,731,430 | 07e1ef2ff8504f70f149bb302571f99215b84570e31d8694d7bce5beac344afa |
| Full downloaded CI evidence ZIP | 2,177,485 | a37db73bcc5eac1561ac733756f6af0c76cf1afd56efc3b45e65d403ddc1171a |
| Accepted R18 IMG (historic) | 287,944 | c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1 |
| Accepted R18 ELF (historic) | 391,712 | c4b3e73e1c1aec13800536b4ef020597848259c421a0b3bf9855e0ddabad8b87 |

The candidate retains the original boot line **LXR LAB R18**, and the generated CI IMG has been archived with the explicit **ARM_VALIDATION_ONLY** designation. It is **not** an authorized replacement image and must not be installed merely because it passed CI. No standalone firmware delivery ZIP was issued.

## Reliability boundaries and next gate

**NOT MEASURED:** CPU cycles/WCET, IRQ/DMA deadlines, missed audio transfers, stack high-water, long-term operation, Cortex-M7 execution of all test vectors, or actual LXR-02 Delay/CLP listening. No hardware flash, merge, release or physical-device test was performed. USB-MIDI and Compressor remain unresolved and out of scope.

Retain the exact candidate SHA and verified evidence. Obtain independent approval before any hardware installation, broader integration, further builds or corrections. Protect the accepted R18 sound and image. The canonical project status and private KnowledgeBase were not changed by this build report.

### Immutable references

- [Candidate R18 diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/41b364067bce0d8cde624c184933d9cb88e2725b...08bc165500e21153eeb28ddbf2fe2d84626a82ca)
- [Successful candidate ARM CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428668)
- [Build evidence artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428668/artifacts/13390187381)
- [Prior Delay source/host report](https://github.com/DocLXR-02/lxr-desktop-app/blob/2c8d7138951e0c7c46b88de602ccc385d8e8ec6b/docs/codex-berichte/2026-11-05_r18-delay-mono-safe-access.md)
- [Accepted R18 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1eee9ed318b592cec54379bcc255927968b936a3/docs/codex-berichte/2026-11-03_r18-clp-v21-soft-build.md)

**Publication:** A report-only branch based on the project documentation branch; no source merge or canonical documentation overwrite. The full raw CI artifact expires unless separately archived in an owner-approved durable location.
