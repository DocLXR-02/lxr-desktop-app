# R4 — Hardware Validation and SD-Card Storage Tests

**Date:** 2026-10-28 (Europe/Vienna) · **Test device:** Daniel's Erica Synths LXR-02 Desktop · **Status:** initial successful hands-on hardware and storage smoke test.

> [!CAUTION]
> **Experimental firmware — no general safety clearance.** R4 has only been tested briefly on **one** LXR-02, without extended use or independent testing across multiple units. A successful smoke test **does not establish** that failures, data loss, instrument damage, damage to connected audio equipment, or hearing injury are impossible. These risks have not been comprehensively assessed. Anyone testing this firmware does so at their own risk and should back up existing projects, keep official manufacturer firmware available for recovery, and **begin audio tests at low volume**. In particular, the **unresolved loud FX-bus `DRV/CLP` whistle** warrants caution.

## Reproducible Firmware Identity

| Item | Reference |
|---|---|
| Custom R4 image | `LXR_V2_V1.7.img`, **288,676 bytes**, SHA-256 `1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877` |
| R4 source branch | [`ui/kit-load-confirmation`](https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation), commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482` |
| R4 delivery | [`delivery/lxr-lab-r4-kit-load-2026-10-28`](https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28), commit `eea1949f0641e375f8cc6082971242549aeed1d1` |
| R4 delivery ZIP | `delivery/LXR_LAB_R4_KIT_LOAD_CONFIRMATION.zip`, SHA-256 `c83290aad78ffa469cd65cec009273c6c29ba029877274d3f59ee44373411a73`, per `delivery/PACKAGE_SHA256SUMS.txt` |
| Official reference image | v1.71 `LXRV2_update_v1.71(1).img`, **273,544 bytes**, SHA-256 `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576` |

The `V1.7` fragment in the custom R4 filename **does not mean** the image is binary-identical to official manufacturer firmware v1.71. Before testing R4, the maintainer successfully installed official v1.71, booted the LXR, and selected both projects present at that time. This **was not** a separately verified rollback *after* R4.

The earlier [R4 packaging report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-r4-firmware-package/docs/codex-berichte/2026-10-28_r4-firmware-package.md) was written **before** hardware testing and therefore stated “not hardware validated.” This dated report adds later evidence without retroactively changing that historical statement.

## Hands-On Hardware Observations

| Test | Result and limitation |
|---|---|
| Flash and boot | **PASS (HARDWARE):** Custom firmware installed; device boots, produces sound, and allows project selection. |
| **Internal Bug 001: Filter Drive morph** | **PASS (HARDWARE):** The previously audible click / Drive jump during kit morphing did not recur. |
| **Internal Bug 002: Filter Drive after pattern change** | **NOT REPRODUCIBLE (HARDWARE):** Checked with AutoSave both enabled and disabled; no separately established cause or fix. |
| Morph control range **127** | **PASS (HARDWARE):** The shorter knob travel felt more comfortable during use. |
| `Load Kit` and `Load Morph Kit` | **PASS (HARDWARE):** Both briefly displayed `success!`. No additional UI changes requested for pattern/save/song workflows. |
| Save Kit/Pattern/Project | **PASS (LIMITED HARDWARE SMOKE TEST):** Multiple saves followed by repeated device reboots; saved content remained available. |
| R4 SD-card archive | **PASS (FILE INSPECTION):** `Archive_resetR4.zip` appeared structurally consistent; details below. |

## SD-Card Artifact Inspection: `Archive_resetR4.zip`

For R4, the maintainer reset the previously troublesome **32 GB FAT32 microSDHC** card and created **new** project folders instead of migrating older ones. A different SD adapter was used, and AutoSave was off. The archive contained:

- `PROJ00` (project name `R4`) and `PROJ01` (`X`), each with **194** data files: **64 SND** files of 255 bytes, **64 PAT** files of 3,663 bytes with `PatV03`, **64 SNG** files of 72 bytes, `GLO.CFG` of 30 bytes, and `PRJ.NFO` of 8 bytes.
- **No zero-byte project data files** in either folder. Both contained plausible edited kit and pattern data.
- `PROJ.IDX` contained `0` and referred to the existing `PROJ00`.
- No unfinished transaction folders. `TMP/TMP.SND` (248 bytes) and `TMP/TMP.PAT` (3,650 bytes) were present and do not by themselves indicate a fault.
- The ZIP structure was readable, and the included R4 image matched the SHA-256 above.

**Result:** Under these conditions, creating new projects, normal save/load use, and reboots worked. This does not demonstrate universal compatibility with older projects or safety against interruption at every stage of an SD write.

## Earlier SD-Card Incident (R3): Observations, Not a Confirmed Cause

The earlier `Archive.zip` showed the following:

1. `PROJ.IDX` was `1`, but **`PROJ01` was absent**. The device showed `SD error / Path? Retry save` at startup.
2. **21 zero-byte project data files** across `PROJ00` and `PROJ14`; other files retained plausible markers and sizes.
3. The SD card's `.img` was only **16,384 bytes** (not a complete custom R4 image; this was the earlier R3 incident).
4. The iPhone Files app intermittently displayed the card as empty while the iPad could see its contents; the files later reappeared on the iPhone.

The R4 retest changed **four factors at once**: card formatting/file-system state, SD adapter, project folders, and AutoSave setting. Accordingly, the R3 incident **cannot yet be attributed** to firmware behavior, interrupted file copies, the card/FAT32, the adapter, or legacy project contents. We make **no claim** that older projects are incompatible, and **no absolute guarantee** that R4 can never corrupt stored data.

## Known Gaps in Validation

- Compatibility of project folders created before R4 **has not** been tested separately.
- No systematic series of real-device power interruptions during writes. Recovery, full-card, and unrelated-file protection checks performed as host tests must **not** be represented as hardware tests.
- No comprehensive long-duration stress testing for audio DMA, CPU peaks, stack use, or arbitrary project data.
- Official v1.71 was successfully installed *before* R4; an R4-to-v1.71 rollback has not been documented.

## Conclusion and Next Steps

**R4 is the current working baseline for this experiment, not a generally certified release.** Changes `LXR-001` and `LXR-003` (historically internal Bugs 001 and 011) were confirmed by the maintainer's hands-on tests. Historical Bug 002 is currently not reproducible, with no independently established causal fix. Future changes should be made on separate branches and backed by traceable source, build, and real-device tests. The next sound-related investigation concerns the persistent **FX-bus `DRV/CLP` whistle**; see [`CHANGELOG.md`](CHANGELOG.md). Related GitHub community work and reproducible hardware conditions should be reviewed first. This independent firmware work was assisted by OpenAI ChatGPT and Codex; roles are described in this documentation branch's README.
