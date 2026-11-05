# R18 Three-Fix Candidate — Owner Device-Test Installation Authorization Addendum

**Recorded date:** 2026-11-05 (Europe/Vienna)  
**Status:** Owner installation and planned device-test approval RECORDED; actual instrument operation NOT OBSERVED or performed by the assistant.  
**Supersedes:** Only the previously unsigned Gate B/C authorization status in the 2026-11-05 [device test, backup and recovery plan](2026-11-05_r18-three-fix-device-test-backup-recovery-plan.md). All other safety gates, stop conditions and recovery limitations remain in force.

## Owner approvals and remaining safety verification

After reading the hardware test plan, the owner stated that the SD card had been backed up and explicitly authorized the test firmware installation and the planned hardware tests. The assistant has not accessed the physical SD card and **cannot independently verify backup completeness, readability, current firmware state, power reliability, DATA encoder condition, warranty/service implications or audio monitoring levels**. The operator must verify these preconditions; any unresolved hard stop in the original plan still blocks flashing. The owner has not reported test outcomes yet.

The owner's desired naming policy is explicit: **retain the musical reference as R18**; the experimental integrated three-fix build is a **TEST** candidate; only after successful owner hardware tests and separate final release approval should a separately validated release be named **R19**, including the intended startup identifier. No R19 source change, build, release or deployment has been performed.

## Exact source and new installation package

| Field | Authorized test-package identity |
|---|---|
| Original, accepted R18 source SHA | `41b364067bce0d8cde624c184933d9cb88e2725b` |
| Three-fix integrated source SHA | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` |
| Passing combined ARM/host CI | [GitHub Actions run 42069839191](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191) |
| Source image archived by CI | `LXR_R18_INTEGRATED_ARM_VALIDATION_ONLY.img` |
| Operator-test installation filename | **`LXR_V2_TEST.img`** |
| Image bytes | **288,184 bytes** |
| Image SHA-256 | **`50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`** |
| Distribution ZIP | **`LXR_V2_TEST.zip`**, 208,017 bytes |
| ZIP SHA-256 | **`c11ab87ca18fbc9ea1572c8ce0c37a3d60236f538c35d1b921ccadacd1281f36`** |
| ZIP entries | Exactly one `.img` plus `SHA256SUMS.txt` and `TEST_INSTALL_NOTES.txt` |
| Package storage | Owner's private Library, `/LXR Firmware Lab/LXR_V2_TEST.zip` (not a public GitHub release) |
| Current boot display text | **`LXR LAB R18`** — ambiguous for distinguishing this test candidate from original R18 |
| Original recovery firmware | `LXR_V2_R18.img`, 287,944 bytes, SHA-256 `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1`; separately retained from original R18 CI |

The distributed `LXR_V2_TEST.img` is **byte-identical** to the previously validated integrated CI IMG; only the external filename was changed to match the manual's `LXR_V2_xxx.img` convention and distinguish TEST from R18/R19. Its actual SHA-256, internal header, declared payload length and internal payload checksum were reverified after copy, followed by ZIP CRC and exact extracted-byte checks. No source modification, cross-build, bootloader edit or new target binary occurred in the package-preparation step.

## Operational limits and evidence

The LXR-02 owner manual section 10.1, page 39, specifies putting the `LXR_V2_xxx.img` in the SD card root, holding the DATA encoder while switching on, waiting for `LXR Bootloader`, allowing size/checksum validation, never interrupting a live firmware update, and rebooting only when prompted. **Only one candidate updater `.img` is to remain in the SD root.** Copy the extracted `.img` rather than the ZIP, do not rename it on the device and preferably verify the SD copy hash.

**Hard stop remains:** The intermittently failing DATA encoder cannot be forced; missing confirmation or uncertain service condition, unsafe monitoring, power/SD errors, missing or unreadable backup, or unverifiable recovery bytes must stop installation. A historical successful R18 update and retained rollback image are **not** evidence that recovery from a failed flash will succeed. Firmware flashing, hardware tests, successful recovery and release are not claimed as executed here; the device operator must provide dated observations.

After installation, record that the **approved SHA-256** was installed, with actual bootloader prompts and observable test outcomes. The displayed `LXR LAB R18` is not evidence of installed image identity by itself. Record each test as PASS/FAIL/INCONCLUSIVE/BLOCKED/NOT RUN using the existing plan. Cortex-M7 CPU/WCET, IRQ/DMA timing margins, stack high-water and long-term reliability remain **UNMEASURED**.

## Release naming — future, conditional

The intended public/accepted successor identifier is **R19**, subject to successful hardware tests, explicit owner sound acceptance, a separately approved source change for the startup string, new matched ARM build and build report, image identity/hash verification and explicit release approval. This addendum does not itself create, label or publish an R19 release.

**Publication scope:** Documentation-only addendum on the separate plan branch. `main`, accepted R18, original integration and the separately verified Delay/USB/Compressor branches remain untouched.
