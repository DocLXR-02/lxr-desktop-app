# LXR Firmware Lab — R19 Final Owner-Approved Firmware Distribution

**Decision date:** 2026-11-08 (Europe/Vienna)  
**Owner decision:** Explicit final approval to publish the **exact previously installed and successfully tested R19 candidate**, without merging to `main`.  
**Current scope:** Owner-approved final R19 binary distribution and public GitHub documentary release record. **No native GitHub Releases entry, tag or attached GitHub release asset was created by this action**; the available connected GitHub interface does not expose creation/upload actions for those. Do not mistake this public record for a verified GitHub Releases page.

## Release and precise identities

| Item | Exact identity |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Final release | **R19** |
| Source branch | `candidate/r19-integrated-three-fix-2026-11-07` |
| **Exact approved release source SHA** | **`638b6eaae5b1624637d20cbc11a4a3e0175edca2`** |
| Integrated technical predecessor SHA | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` |
| Protected, musically accepted original R18 source SHA | `41b364067bce0d8cde624c184933d9cb88e2725b` |
| Built startup string | `LXR LAB R19` |
| **Final firmware file** | **`LXR_V2_R19.img`** |
| **Firmware size** | **288,184 bytes** |
| **Firmware SHA-256** | **`a1d53435ce922c4ef57ce595bf8c180b5225340780b6ad3241b6340af652e57b`** |
| **Final distribution ZIP** | **`LXR_V2_R19.zip`** (exactly one IMG) |
| **ZIP size** | **206,167 bytes** |
| **ZIP SHA-256** | **`b9c25355a0a880b4162ecfc3d439f784593f134bb21132e6d749d06103f01068`** |
| Release-record ZIP | `LXR_R19_Release_Record_2026-11-08.zip`, SHA-256 `b05d2960d7763adffa91bb9010b35cda6520ce0e4b02a6ca6408ca0133969279` |
| Successful Release ARM build | [GitHub Actions 42407160858](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858), attempt 1, SUCCESS |
| Independent per-build technical report | [2026-11-07 R19 candidate ARM build](https://github.com/DocLXR-02/lxr-desktop-app/blob/ce2d87d499a5708881f38b0fe0ffa6b0e50c28f0/docs/codex-berichte/2026-11-07_r19-version-only-candidate-arm-build.md) |
| Owner device acceptance | [2026-11-08 hardware installation and functional test](https://github.com/DocLXR-02/lxr-desktop-app/blob/dcd3b3c0c137259d38bd1cb9eac8bbe0622ba784/docs/hardware/2026-11-08_r19-candidate-owner-device-acceptance.md) |

## What R19 changes

Relative to original accepted R18, the owner-approved R19 binary contains three separately developed and jointly validated corrections: Delay mono-safe access, USB-MIDI FIFO/transfer ownership, and Compressor mono output over-range saturation. The accepted R18 FX-CLP musical behavior, oscillators, effect routing, kit/pattern formats and bootloader remain unchanged by this final step.

Relative to the already owner-installed three-fix integration TEST firmware, the R19 firmware differs by **exactly two bytes**: one embedded version digit (`LXR LAB R18` to `LXR LAB R19`) and the corresponding updater checksum byte. This has been independently confirmed against the CI source artifact. There is **no new firmware build** for final distribution, no recompiled code, and no firmware source change after the owner-tested R19 candidate.

The R19 distribution filename was changed from `LXR_V2_R19_CANDIDATE.img` to `LXR_V2_R19.img` without touching any image byte; SHA-256 of the final renamed entry was independently confirmed. The final ZIP has exactly one IMG, and ZIP CRC, extracted bytes, application header and internal checksum were verified. The final package and a separate English release-record ZIP were preserved in the owner's **private Library** at `/LXR Firmware Lab/`. They were also attached to the owner's conversation for download. The private Library is **not** a public GitHub Releases hosting service.

## Owner tests, consciously accepted limitations

- Owner reported successful physical installation and a function test of the exact R19 candidate on **2026-11-08**, with no problem reported. Specific test duration, full matrix and instrument readback hash were **not supplied**. Do not present this as exhaustive certification.
- Prior owner listening tests of the technically integrated TEST candidate confirmed accepted FX-CLP musical character, multi-voice routing, and Delay normal/ping-pong. The transient clicking across FX types, also observed on historic/vendor firmware, disappeared after project reload, including the previously affected project. The transient root cause is **UNKNOWN** and not established as a Compressor regression.
- Physical **USB-MIDI device validation: NOT RUN/BLOCKED** for unavailable equipment. The owner **explicitly accepted this as a nonblocking final release risk**. USB-MIDI source-extracted host tests and CI passed; do not label physical USB-MIDI as PASS. The bounded TX/RX ring can drop incoming/new messages at capacity, potentially including Note-Off under sustained overflow.
- **UNMEASURED:** CPU/WCET, IRQ/DMA buffer deadlines, actual stack high-water, long-run/thermal stability. Static headroom figures are not equivalent to runtime margin.
- The R18 original firmware/image and full SD data backup should be preserved separately before any future device update. Recovery/downgrade after a failed flash is **not guaranteed**. For other devices follow the manufacturer's SD application update procedure, with stable power, verified IMG checksum and low monitored audio volume. Do not update the bootloader.

## Release permissions and repository boundaries

The owner explicitly approved **final R19 publication of the existing tested image**, without merging to `main`. No further user authorization is required to label this exact image as final R19 or issue the already-validated download package. This approval **does not** authorize later DSP changes, further builds, editing accepted R18, bootloader updates, force pushes, or merging into `main`.

The connected GitHub interface presently supports publishing this documentation but **does not expose creation of a native GitHub Release/tag or binary asset upload**. A public GitHub Releases entry and hosted asset remain **NOT CREATED/UNVERIFIED**, not falsely marked complete. If a native GitHub Release is later created via an authorized external GitHub web workflow, it must reference this exact source SHA and the IMG/ZIP checksums above. Do not rebuild or silently substitute files.

**This is the authoritative final-owner-approved R19 distribution record and supersedes the earlier R19-candidate-pending-approval status, while retaining the complete historical evidence.**
