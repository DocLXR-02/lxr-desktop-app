# LXR Firmware Lab — Phase R2: Verified Firmware Packaging and iPhone Download

Date: **2026-10-28 UTC**. Repository: **DocLXR-02/lxr-desktop-app**.

## 1. Outcome

**COMPLETE — exact approved Release image rebuilt, verified, packaged and downloaded back from GitHub successfully.** No image substitution occurred. No firmware source, DSP, build configuration, bootloader or hardware setting was changed. No device was flashed and no physical SD card was accessed or modified.

The delivered image is **LXR_V2_V1.7.img, 288,612 bytes**, SHA-256:

```text
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76
```

This exactly matches the user's expected identity and the R1-reviewed image. Diagnostics and UPDATE_BOOTLOADER remain disabled. The ZIP is an unofficial laboratory delivery, not a GitHub Release or official manufacturer firmware release. **NOT FLASHED — NOT HARDWARE VALIDATED. Installation still requires separate explicit user approval.**

## 2. Direct downloads and exact references

**Pinned direct ZIP download (verified by actual unauthenticated HTTP GET):**

https://github.com/DocLXR-02/lxr-desktop-app/raw/2c2173957bea27f9eb0bd2e10e70115268aa74c7/delivery/LXR_LAB_R1_FIRMWARE.zip

Equivalent verified branch download:

https://github.com/DocLXR-02/lxr-desktop-app/raw/refs/heads/delivery/lxr-lab-r1-2026-10-28/delivery/LXR_LAB_R1_FIRMWARE.zip

Pinned archive manifest:

https://github.com/DocLXR-02/lxr-desktop-app/raw/2c2173957bea27f9eb0bd2e10e70115268aa74c7/delivery/SHA256SUMS.txt

Delivery browser page:

https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r1-2026-10-28/delivery

| Item | Exact identity |
|---|---|
| Approved source branch | `safety/sd-project-transaction-s4b2` |
| Approved and rebuilt source commit | **`5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527`** |
| Approved source Git tree | `aca77103ba2509431cc966febc0ec6468defac6a` |
| New delivery branch | **`delivery/lxr-lab-r1-2026-10-28`** |
| Delivery commit | **`2c2173957bea27f9eb0bd2e10e70115268aa74c7`** |
| Delivery commit parent | `5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527` (one parent) |
| Delivery Git tree | `f6866eac249a378efba75d4ae7c174ae240e0c64` |
| New report branch | `codex-reports/2026-10-28-phase-r2-firmware-delivery` |
| New report path | `docs/codex-berichte/2026-10-28_phase-r2-verified-firmware-packaging.md` |
| Report-only base | `lab-setup`, `5829398cea5d337200393757e39a37d4d8148256` |
| Preserved main | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |

The report's containing commit is identified by GitHub metadata/final handoff rather than embedding its future self-hash recursively in its content. The report branch is separate from the binary-delivery branch and does not include its payload.

## 3. Instructions, starting state and scope

The complete `AGENTS.md` was read from lab-setup. The existing checkout `/workspace/lxr-desktop-app` was clean at the exact approved source commit before building. The current remote source head matched5ab5e6b1 exactly. A37-branch remote snapshot was recorded before publication; all pre-existing heads were compared unchanged during publication and are checked again after this report.

The complete R1 report was fetched from:

https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-phase-r1-hardware-readiness/docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md

R1 report commit: `46fd1d138f80f7ab8e382b7f33230dd6abab3285`; file blob `036253ab39ebc7de29bc4136eea56f0dda3ed9ae`. R1 gives **GO for controlled hardware test preparation**, not automatic installation. Its artifact restriction, recovery prerequisites and unmeasured audio/storage/stack limitations remain applicable.

This R2 task explicitly authorizes publication of the verified ZIP on a separate delivery branch if an unchanged existing Actions mechanism is unsuitable. It does not authorize device flashing, original-card use, further source changes or a manufacturer release. No new regression tests, feature, version change or source branch was created. The implementation checkout remains on the approved source branch, clean; publication uses separate new GitHub refs. No worktree, merge, force-push or overwrite of existing reports/files occurred.

The cloud-environment runtime skill and current environment/network policy were checked. The observed policy was enforced/current; no credential values were inspected or included. Temporary scripts, manifests, logs and downloaded verification copies reside under `/tmp/phase-r2`; generated firmware outputs remain ignored under `build/release`. The earlier `/tmp/phase-r1` runtime logs were no longer present in this resumed environment, so R2 re-executed the build/identity checks and consulted the published R1 record rather than claiming old temporary files were available.

## 4. Build configuration and exact executed commands

Environment activation:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Preconditions checked before compiling:

```text
git status --short --branch
  ## safety/sd-project-transaction-s4b2
  (no modified/untracked project files)
git rev-parse HEAD
  5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527
git rev-parse HEAD^{tree}
  aca77103ba2509431cc966febc0ec6468defac6a
```

Prepared tools actually reported:

| Tool | Version |
|---|---|
| CMake | 3.31.10 |
| Ninja | 1.13.0.git.kitware.jobserver-pipe-1 |
| Python | 3.12.14 |
| ARM GCC | 14.2.1 20241119, package15:14.2.rel1-1 |
| Packaging zlib | 1.3.2 (build/runtime) |

Successful commands, all exit0:

```sh
cmake --version
ninja --version
python3 --version
arm-none-eabi-gcc --version
cmake --preset release
cmake --build --preset release --parallel 4 --clean-first
python3 /tmp/phase-r2/verify.py
python3 tests/euclidean/check_images.py build/release
python3 /tmp/phase-r2/package.py
# In /tmp/phase-r2/delivery:
sha256sum -c SHA256SUMS.txt
python3 /tmp/phase-r2/verify_download.py
```

`--clean-first` removed only generated build outputs and recompiled the existing Release preset; it did not alter tracked configuration. Actual configuration remained:

- `CMAKE_BUILD_TYPE=Release`, existing `-O3` application configuration.
- `LXR_RUNTIME_DIAGNOSTICS=OFF`.
- `LXR_BOOTLOADER_DIR` and `LXR_BOOTLOADER_HEX` empty.
- Existing STM32F765/Cortex-M7/hard-float toolchain and linker script.
- Effective preprocessed `#define UPDATE_BOOTLOADER 0` in main.c, using its actual Release compilation command.

The configuration log explicitly reports application-only output with no configured bootloader/full-flash image. Neither a flashing target nor bootloader target was invoked. The normal build prints existing update instructions as informational text; none were executed. No new ARM compiler warning/error was found.

Development errors were limited to tooling orchestration: the first build-driver launch omitted the prepared PATH and stopped with `FileNotFoundError: cmake` before compilation; it was rerun after correct activation. A preliminary download verifier read an empty in-progress HTTP metadata file and stopped with JSONDecodeError; the outstanding download process was then awaited and the complete verification rerun passed. Neither incident produced an alternative firmware or a size/hash mismatch. The required STOP-on-image-discrepancy condition was never triggered: the rebuilt image matched immediately when checked.

R2 ran the Release build and image/package validation requested here. It did **not** rerun R1's twelve functional suites or four configurations; those remain the recorded results for this identical source/image in the published R1 report. No test was modified or weakened.

## 5. Independent image and app-only validation

Two checks were executed: the established committed `tests/euclidean/check_images.py` and a separate temporary verifier (reproduced below) that additionally enforces the exact expected SHA/size, effective configuration and all load-section addresses.

| Check | Actual result |
|---|---|
| Exact approved HEAD and source tree | PASS |
| Tracked checkout clean before/after build | PASS |
| Image bytes | **288,612**, exact expected |
| Image SHA-256 | Exact expected `00b75f...79d76` |
| Header | 16 bytes, `LXRV2IMG` |
| Header hex | `4c58525632494d4754670400bb000000` |
| LE32 payload length | **288,596** (`0x00046754`) |
| Checksum byte | **BB** |
| Checksum convention | `(sum(payload) + 0xBB) & 0xFF == 0xFF` |
| Reserved header padding | Three zero bytes |
| Complete binary correspondence | `image[16:]` equals the entire generated `.bin`, byte-for-byte |
| Vector initial MSP | `0x20080000`, expected desktop stack vector |
| Reset vector | `0x080154E1`, Thumb, within application image |
| First actual load section / application origin | `0x08008000` |
| Highest load end, exclusive | `0x0804E754` |
| All ALLOC sections | Within their configured Flash/RAM/TCM regions |
| All LOAD section LMAs | Within application Flash; highest span equals `.bin` size |
| Bootloader binary in payload | None; payload is the section-derived application binary |
| Bootloader update code enabled | No, effective UPDATE_BOOTLOADER=0 |
| Configured bootloader/full image inputs | None |

Only the app `.img` is packaged. No ELF, raw BIN, HEX, bootloader or full-device image is distributed. The R1 ELF program-header/alignment caveat therefore does not enter this ZIP. “No bootloader programming content” here means no bootloader image/full-flash prefix and no enabled application bootloader-update branch; it is not a formal proof that arbitrary execution can never write Flash. Normal application update destination remains0x08008000. Physical bootloader/protection state was not inspected or changed.

Actual local artifact identities:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_V1.7.img` | 288,612 | `00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76` |
| `LXR_V2_STM32F765.bin` | 288,596 | `86169ccaa31d74ff997aa5b69f2fcfe6cf61e0aeed29e8b926607ca9b552c7bd` |
| `LXR_V2_STM32F765.elf` (not distributed) | Not used as installation payload | `57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed` |

These image/bin/ELF hashes reproduce the approved R1 identities exactly. The small additive image checksum is the updater format check; full-file SHA-256 provides stronger identity checking but is not a manufacturer signature or hardware validation.

### Static memory

| Region / metric | Used | Capacity | Remaining |
|---|---:|---:|---:|
| ITCM | 15,016 | 16,384 | 1,368 |
| DTCM static high-water | 107,392 | 131,072 | 23,680 |
| General RAM linker usage | 274,640 | 393,216 | 118,576 |
| Full application Flash load span | 288,596 | 1,015,808 | 727,212 |

Linker-reported FLASH region usage is236,584 bytes. Section FLASH VMA high-water is235,732 bytes; neither is the entire Flash LMA span because explicit TCM initializer load data must also fit. The conservative full-span check passes. All values match R1; packaging adds zero firmware RAM, Flash, CPU or stack cost. Runtime stack high-water and timing are not measured by these numbers.

## 6. Actions assessment and authorized fallback

The existing workflow `.github/workflows/lxr-build.yml` was inspected unchanged at the approved commit. It triggers only on pushes to `lab-setup`, has no `workflow_dispatch`, installs **ARM GNU14.3.rel1**, and uploads a seven-day artifact containing firmware plus ELF/map/build information. It does not build this specified source through an available unchanged dispatch path, does not package the requested three-entry ZIP, and its compiler differs from the14.2.1 environment producing the approved SHA.

The actual Actions runs query:

```text
GET /repos/DocLXR-02/lxr-desktop-app/actions/runs?head_sha=5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527&per_page=20
{"total_count":0,"workflow_runs":[]}
```

No workflow was added, modified, dispatched or rerun. There is no claimed Actions run/artifact for this delivery. The explicitly authorized fallback was used: a **new delivery-only commit based directly on the approved source commit**, adding exactly:

```text
delivery/LXR_LAB_R1_FIRMWARE.zip
delivery/SHA256SUMS.txt
delivery/README.txt
```

No existing source file differs from its parent. No loose firmware `.img`, raw BIN, ELF, build directory or source change was added. Git-data APIs uploaded exact byte blobs, verified their Git SHAs, created a tree over the approved tree, created a one-parent commit and created only the new delivery ref. A fresh remote commit/ref read confirmed parent, tree and exact three-file diff. The implementation branch was not advanced to the delivery commit. The repository is public; actual downloads were tested without authentication. No GitHub Release was created and there is no automatic firmware installation mechanism in this delivery.

## 7. ZIP format, contents and manifests

Archive filename: **`LXR_LAB_R1_FIRMWARE.zip`**.

Archive size: **208,402 bytes**.

Archive SHA-256:

```text
ae207c27a5015145923a6f494770b90197681cb2b6deda60a549e746c3d01b7a
```

The standard non-encrypted ZIP uses DEFLATE level9, zlib1.3.2, fixed DOS timestamp2026-10-28 00:00:00, fixed member order, ASCII filenames and regular DOS archive attributes. The fixed timestamp is packaging metadata, not a claimed instrument clock measurement. Two independently generated archives from the same input bytes were identical. There are no absolute paths, nested archives, scripts, symlinks, hidden `__MACOSX` files or extra firmware images.

Exactly three ZIP members, in order:

| ZIP member | Uncompressed bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_V1.7.img` | 288,612 | `00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76` |
| `SHA256SUMS.txt` | 159 | `d8f23c78e97aef6151aea7354d47761fc3916d5ee0b037b2b44df5081911ab94` |
| `README.txt` | 3,461 | `631e56b5c318ea690c1f7561f0978c0ede0574fc9ac3076c092cea5a2fb4b7da` |

Inside manifest:

```text
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76  LXR_V2_V1.7.img
631e56b5c318ea690c1f7561f0978c0ede0574fc9ac3076c092cea5a2fb4b7da  README.txt
```

The separate delivery-folder `SHA256SUMS.txt` is intentionally different: it checks the downloadable archive itself, avoiding a self-referential ZIP checksum:

```text
ae207c27a5015145923a6f494770b90197681cb2b6deda60a549e746c3d01b7a  LXR_LAB_R1_FIRMWARE.zip
```

External manifest size90 bytes, SHA-256 `56dd11d8e366966b2386207fce98c972471ac8a4ab16af4f3f1b3d034ed88a6b`. The standalone delivery README is byte-identical to the README inside the ZIP. It identifies source/branch/commit, intended Desktop STM32F765VG hardware, Release/OFF, exact image size/hash, unofficial modified status, corresponding source/license, manufacturer public v1.71 recovery reference, no hardware validation and separate approval requirement. It does not claim public-source binary equivalence or guaranteed recovery.

README uses plain ASCII/CRLF for simple mobile viewing; manifests use conventional SHA-256 lines. Both local and downloaded ZIPs passed complete member CRC checks and exact member-byte/hash comparison. The member name list and case-insensitive `.img` count were asserted, with exactly one image. The compressed copy is byte-identical to the rebuilt image after extraction.

## 8. Actual download verification and iPhone use

The pinned GitHub download was retrieved through HTTPS with curl, no login/cookie/authentication options, TLS verification enabled:

```sh
curl --fail --location --silent --show-error --max-time 45 \
  --output /tmp/phase-r2/downloaded-commit.zip --write-out '%{json}\n' \
  https://github.com/DocLXR-02/lxr-desktop-app/raw/2c2173957bea27f9eb0bd2e10e70115268aa74c7/delivery/LXR_LAB_R1_FIRMWARE.zip
```

The branch URL and pinned external manifest were fetched separately. Each curl operation exited0. Both ZIP URLs returned **HTTP200**, `Content-Type: application/zip`,208,402 bytes, one redirect to `raw.githubusercontent.com`, TLS verify result0. The manifest returned HTTP200, text/plain,90 bytes. Downloaded bytes matched the local archive/manifest exactly; then the inner image and README hashes/manifests were rechecked. This is verification of an actual binary download, not merely a GitHub file listing, API success or prospective URL.

For iPhone: open the pinned ZIP download link in Safari, then in Files → Downloads tap the archive to extract it. It is a small standard ZIP with three simple filenames; extracting does not install firmware. The Files app's displayed filename/size is not SHA verification: use a trusted checksum tool or computer before any separately approved installation. Keep the original SD card untouched. Do not place the ZIP itself on a card as if it were an update image.

**Limitation:** cloud HTTP/ZIP verification is real, but this task did not operate an actual iPhone/Safari instance. “iPhone-friendly” describes the standard archive/direct public download, not a physical-device UI test. Network availability is verified at publication time, not promised indefinitely. The pinned commit URL avoids branch drift; SHA-256 still should be checked after download/copy.

## 9. Outstanding hardware and approval boundaries

Packaging does not change R1's readiness or grant installation permission. Still required before/at an approved first session:

- Separate prepared test card, verified independent backup of the original card, including explicit treatment of the unresolved PROJ00 observation.
- Known authentic manufacturer v1.71 Desktop recovery image, obtained and verified separately; it is not in this ZIP and R2 did not obtain it. Confirm intact compatible device updater/procedure; no guaranteed recovery from bootloader/hardware damage is asserted.
- Daniel's explicit approval for this exact source/image and a controlled session.
- Actual startup/display/controls/audio/sequencer and basic MIDI/USB checks.
- Original Bug001 Voice1 Bass Kick, identical kits, no LFO/automation, Drive0/127, full Morph range/endpoints/stationary comparison and relevant filter modes.
- Disposable project create/save/reboot/reload/repeated replacement and observation of temporary transaction cleanup.
- Audio/UI/MIDI behavior and save/load latency under normal use; runtime stack high-water/diagnostic interpretation remain unmeasured. No deliberate power-loss test in the first session.

The complete procedure and conditional recovery/stop guidance remain in the linked R1 report. No additional feature/refactor, binary substitution, bootloader setting, SD repair or diagnostic exporter was introduced to deliver this package.

## 10. Publication receipt and continuation

Delivery branch/commit and actual anonymous ZIP download have been verified. All37 pre-existing branch heads remain unchanged; the new report is verified separately after creation. The report-only branch has lab-setup as parent and adds only this Markdown file. The report's containing commit SHA is reported in GitHub metadata/final handoff; a future self-commit SHA is not embedded recursively.

To continue: use the pinned download and hashes above, read README/R1, satisfy the checklist, then obtain separate installation approval. Do not rebuild with another compiler or substitute an image just because its version filename is1.7. Any future size/SHA discrepancy from the approved image requires stopping and investigation.

**No firmware source code or build configuration was modified. No device was flashed. No physical SD-card content was accessed or modified.**

## Appendix A. Actual build results

```json
[
  {
    "command": "cmake --version",
    "exit": 0
  },
  {
    "command": "ninja --version",
    "exit": 0
  },
  {
    "command": "python3 --version",
    "exit": 0
  },
  {
    "command": "arm-none-eabi-gcc --version",
    "exit": 0
  },
  {
    "command": "cmake --preset release",
    "exit": 0
  },
  {
    "command": "cmake --build --preset release --parallel 4 --clean-first",
    "exit": 0
  }
]
```

```text
$ cmake --version
cmake version 3.31.10

CMake suite maintained and supported by Kitware (kitware.com/cmake).

EXIT 0

```

```text
$ ninja --version
1.13.0.git.kitware.jobserver-pipe-1

EXIT 0

```

```text
$ python3 --version
Python 3.12.14

EXIT 0

```

```text
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


EXIT 0

```

```text
$ cmake --preset release
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/release

EXIT 0

```

Clean build resource output:

```text
$ cmake --build --preset release --parallel 4 --clean-first
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      274640 B       384 KB     69.84%
           FLASH:      236584 B       992 KB     23.29%
   text	   data	    bss	    dec	    hex	filename
 287736	    852	 344176	 632764	  9a7bc	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 288596 bytes, checksum 0xBB
[123/123] Firmware update instructions

EXIT 0
```

## Appendix B. Full independent image verification

```json
{
  "source_commit": "5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527",
  "source_branch": "safety/sd-project-transaction-s4b2",
  "git_tree": "aca77103ba2509431cc966febc0ec6468defac6a",
  "configuration": "Release",
  "diagnostics": false,
  "update_bootloader": 0,
  "bootloader_inputs_configured": false,
  "filename": "LXR_V2_V1.7.img",
  "image_bytes": 288612,
  "image_sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76",
  "magic": "LXRV2IMG",
  "header_hex": "4c58525632494d4754670400bb000000",
  "payload_bytes": 288596,
  "checksum": "0xbb",
  "payload_matches_bin": true,
  "bin_sha256": "86169ccaa31d74ff997aa5b69f2fcfe6cf61e0aeed29e8b926607ca9b552c7bd",
  "elf_sha256": "57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed",
  "initial_msp": "0x20080000",
  "reset_vector": "0x80154e1",
  "application_origin": "0x8008000",
  "highest_flash_load_exclusive": "0x804e754",
  "flash_headroom": 727212,
  "memory_section_high_water": {
    "ITCM": 15016,
    "DTCM": 107392,
    "RAM": 274640,
    "FLASH": 235732
  },
  "sections": [
    {
      "name": ".isr_vector",
      "size": 504,
      "vma": "0x8008000",
      "lma": "0x8008000",
      "load": true
    },
    {
      "name": ".text",
      "size": 152680,
      "vma": "0x8008200",
      "lma": "0x8008200",
      "load": true
    },
    {
      "name": ".rodata",
      "size": 82524,
      "vma": "0x802d668",
      "lma": "0x802d668",
      "load": true
    },
    {
      "name": ".ARM",
      "size": 8,
      "vma": "0x80418c4",
      "lma": "0x80418c4",
      "load": true
    },
    {
      "name": ".init_array",
      "size": 4,
      "vma": "0x80418cc",
      "lma": "0x80418cc",
      "load": true
    },
    {
      "name": ".fini_array",
      "size": 4,
      "vma": "0x80418d0",
      "lma": "0x80418d0",
      "load": true
    },
    {
      "name": ".data",
      "size": 852,
      "vma": "0x20020000",
      "lma": "0x80418d4",
      "load": true
    },
    {
      "name": ".bss",
      "size": 273272,
      "vma": "0x20020358",
      "lma": "0x8041c28",
      "load": false
    },
    {
      "name": "._user_heap",
      "size": 512,
      "vma": "0x20062ed0",
      "lma": "0x8041c28",
      "load": false
    },
    {
      "name": ".dtcm",
      "size": 36996,
      "vma": "0x20000000",
      "lma": "0x8041c28",
      "load": true
    },
    {
      "name": ".dtcmz",
      "size": 70392,
      "vma": "0x20009088",
      "lma": "0x804acb0",
      "load": false
    },
    {
      "name": ".itcm_text",
      "size": 15016,
      "vma": "0x0",
      "lma": "0x804acac",
      "load": true
    }
  ],
  "tracked_worktree_clean": true
}
```

Established check:

```text
{"checksum": "0xBB", "configuration": "release", "elf_sha256": "57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed", "image_bytes": 288612, "image_sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235732, "ITCM_RAM": 15016, "RAM": 274640}, "payload_bytes": 288596, "reset_vector": "0x080154E1", "result": "PASS"}

```

## Appendix C. Package/member and HTTP metadata

```json
{
  "zip_filename": "LXR_LAB_R1_FIRMWARE.zip",
  "zip_bytes": 208402,
  "zip_sha256": "ae207c27a5015145923a6f494770b90197681cb2b6deda60a549e746c3d01b7a",
  "zip_timestamp": [
    2026,
    10,
    28,
    0,
    0,
    0
  ],
  "zip_compression": "deflate level 9",
  "zlib_version": "1.3.2",
  "zlib_runtime": "1.3.2",
  "deterministic_two_builds_equal": true,
  "firmware_img_count": 1,
  "zip_entries": [
    {
      "name": "LXR_V2_V1.7.img",
      "bytes": 288612,
      "sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76"
    },
    {
      "name": "SHA256SUMS.txt",
      "bytes": 159,
      "sha256": "d8f23c78e97aef6151aea7354d47761fc3916d5ee0b037b2b44df5081911ab94"
    },
    {
      "name": "README.txt",
      "bytes": 3461,
      "sha256": "631e56b5c318ea690c1f7561f0978c0ede0574fc9ac3076c092cea5a2fb4b7da"
    }
  ],
  "delivery_files": [
    {
      "path": "delivery/LXR_LAB_R1_FIRMWARE.zip",
      "mode": "100644",
      "type": "blob",
      "bytes": 208402,
      "sha": "12373caede8a1adad9c5bcda8d4e1beb12a94976",
      "sha256": "ae207c27a5015145923a6f494770b90197681cb2b6deda60a549e746c3d01b7a"
    },
    {
      "path": "delivery/SHA256SUMS.txt",
      "mode": "100644",
      "type": "blob",
      "bytes": 90,
      "sha": "dd6a0bc412d6d94690d1c5d3dc390d8f0c407fe1",
      "sha256": "56dd11d8e366966b2386207fce98c972471ac8a4ab16af4f3f1b3d034ed88a6b"
    },
    {
      "path": "delivery/README.txt",
      "mode": "100644",
      "type": "blob",
      "bytes": 3461,
      "sha": "ef7930b03ea77965f6ce4eef7684ef6d4db39f14",
      "sha256": "631e56b5c318ea690c1f7561f0978c0ede0574fc9ac3076c092cea5a2fb4b7da"
    }
  ]
}
```

```text
{
  "commit": {
    "http_code": 200,
    "content_type": "application/zip",
    "size_download": 208402,
    "url_effective": "https://raw.githubusercontent.com/DocLXR-02/lxr-desktop-app/2c2173957bea27f9eb0bd2e10e70115268aa74c7/delivery/LXR_LAB_R1_FIRMWARE.zip",
    "num_redirects": 1,
    "ssl_verify_result": 0
  },
  "branch": {
    "http_code": 200,
    "content_type": "application/zip",
    "size_download": 208402,
    "url_effective": "https://raw.githubusercontent.com/DocLXR-02/lxr-desktop-app/refs/heads/delivery/lxr-lab-r1-2026-10-28/delivery/LXR_LAB_R1_FIRMWARE.zip",
    "num_redirects": 1,
    "ssl_verify_result": 0
  }
}
PASS anonymous GitHub commit and branch ZIP downloads, exact archive bytes, CRC, both manifests and exact one approved image; checkout unchanged

```

## Appendix D. Exact packaged README

```text
LXR FIRMWARE LAB R1 - VERIFIED TEST PACKAGE
UNOFFICIAL MODIFIED FIRMWARE - NOT A MANUFACTURER RELEASE

Intended hardware: Erica Synths LXR-02 Desktop, STM32F765VG.
NOT intended for the LXR Eurorack module or other hardware.

Source repository: https://github.com/DocLXR-02/lxr-desktop-app
Source branch: safety/sd-project-transaction-s4b2
Exact source commit: 5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527
Corresponding source and license:
https://github.com/DocLXR-02/lxr-desktop-app/tree/5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527
See LICENSE and NOTICE.md there. GPL-3.0-only with the stated terms.

Build: Release (-O3), runtime diagnostics OFF.
Toolchain: ARM GCC 14.2.1 20241119, prepared cloud toolchain.
CMake 3.31.10; Ninja 1.13.0.git.kitware.jobserver-pipe-1;
Python 3.12.14. Repository firmware version string: 1.7.

Firmware file: LXR_V2_V1.7.img
Exact file size: 288612 bytes
SHA-256:
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76

This is an application-only desktop SD update image (LXRV2IMG).
Payload starts at application flash address 0x08008000.
The bootloader binary is not included; UPDATE_BOOTLOADER=0.
No ELF, raw BIN, HEX, bootloader or full-device image is included.
The ZIP contains exactly one firmware .img file.

IMPORTANT APPROVAL AND HARDWARE STATUS
This package has NOT been tested on physical hardware.
Installation requires separate, explicit user approval for this exact
image and controlled session. Downloading or extracting it is not approval.
No device was flashed during building, verification or packaging.
Do not use or modify the user's original SD card for the experiment.
Use separate prepared test media and verified independent backups.

Recovery reference: manufacturer's public LXR-02 Desktop firmware v1.71.
https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/
The recovery image is NOT included. Obtain and verify it before approved
installation. Repository version 1.7 is not claimed equivalent to public
v1.71 or a privately supplied v1.72 test image. Recovery through the SD
updater depends on an intact compatible installed bootloader; it is not
a guaranteed recovery from every hardware/bootloader failure.

Review and first-session plan (R1):
https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-phase-r1-hardware-readiness/docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md
No installation procedure is executed or newly authorized by this package.

IPHONE DOWNLOAD AND EXTRACTION
Download LXR_LAB_R1_FIRMWARE.zip in Safari. In Files > Downloads, tap the
ZIP to extract it. The extracted folder must contain exactly:
  LXR_V2_V1.7.img
  SHA256SUMS.txt
  README.txt
Read this file before taking further action. Extracting does not install.
Keep the image filename unchanged. Verify the entire image SHA-256 with
a trusted checksum tool (or a computer) before an approved installation;
the Files app's filename/size display alone is not a checksum check.
Do not mistake the ZIP itself for an SD firmware update image.

MANIFESTS
SHA256SUMS.txt beside the downloadable ZIP checks the ZIP archive.
SHA256SUMS.txt inside the ZIP checks the firmware image and this README.
No timestamp, CPU load, audio reliability or power-loss safety is claimed
as measured on an instrument. Normal audio, Bug 001, MIDI/USB, project
save/reload and recovery behavior still require approved physical testing.

```

## Appendix E. Reproducible temporary scripts

These scripts were executed outside tracked source, not committed to the implementation or delivery branches.

### verify.py

```python
from pathlib import Path
import hashlib,json,re,shlex,struct,subprocess
root=Path('/workspace/lxr-desktop-app');out=Path('/tmp/phase-r2');d=root/'build/release'
expected='5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()==expected
assert not subprocess.check_output(['git','status','--porcelain'],cwd=root)
image=(d/'LXR_V2_V1.7.img').read_bytes();binary=(d/'LXR_V2_STM32F765.bin').read_bytes()
sha=hashlib.sha256(image).hexdigest()
assert len(image)==288612 and sha=='00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76','STOP: unapproved image identity'
magic,n,checksum,padding=struct.unpack('<8sIB3s',image[:16])
assert magic==b'LXRV2IMG' and padding==b'\0'*3
assert n==len(binary)==len(image)-16 and image[16:]==binary
assert (sum(binary)+checksum)&255==255
msp,reset=struct.unpack_from('<II',binary)
assert msp==0x20080000 and reset&1 and 0x08008000<=reset&~1<0x08008000+len(binary)
cache=(d/'CMakeCache.txt').read_text()
for x in ['CMAKE_BUILD_TYPE:STRING=Release','LXR_RUNTIME_DIAGNOSTICS:BOOL=OFF','LXR_BOOTLOADER_DIR:PATH=\n','LXR_BOOTLOADER_HEX:FILEPATH=\n']:
 assert x in cache,x
# Verify the effective macro in the actual release main.c compilation context.
commands=json.loads((d/'compile_commands.json').read_text())
item=next(x for x in commands if x['file'].endswith('/Core/Src/main.c'))
cmd=shlex.split(item['command']);cmd.remove('-c');cmd[cmd.index('-o')+1]=str(out/'main-macros.txt');cmd+=['-dM','-E']
subprocess.run(cmd,cwd=item['directory'],check=True,capture_output=True)
macros=(out/'main-macros.txt').read_text()
assert re.search(r'^#define UPDATE_BOOTLOADER 0$',macros,re.M)
assert not re.search(r'^#define LXR_RUNTIME_DIAGNOSTICS (?!0$)',macros,re.M)
listing=subprocess.check_output(['arm-none-eabi-objdump','-h',str(d/'LXR_V2_STM32F765.elf')],text=True)
regions={'ITCM':(0,16384),'DTCM':(0x20000000,131072),'RAM':(0x20020000,393216),'FLASH':(0x08008000,1015808)}
high={k:0 for k in regions};end=0;sections=[]
for m in re.finditer(r'^\s*\d+\s+(\S+)\s+([\da-f]+)\s+([\da-f]+)\s+([\da-f]+)[^\n]*\n([^\n]*)',listing,re.M):
 name,size,vma,lma,flags=m.groups();size,vma,lma=map(lambda x:int(x,16),[size,vma,lma])
 if not size or 'ALLOC' not in flags:continue
 matched=[(k,b,c) for k,(b,c) in regions.items() if b<=vma and vma+size<=b+c];assert len(matched)==1,name
 k,b,c=matched[0];high[k]=max(high[k],vma+size-b)
 if 'LOAD' in flags:
  assert 0x08008000<=lma and lma+size<=0x08100000
  end=max(end,lma+size-0x08008000)
 sections.append(dict(name=name,size=size,vma=hex(vma),lma=hex(lma),load='LOAD' in flags))
assert end==len(binary)
assert next(x for x in sections if x['name']=='.isr_vector')['lma']=='0x8008000'
meta=dict(source_commit=expected,source_branch='safety/sd-project-transaction-s4b2',git_tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=root,text=True).strip(),configuration='Release',diagnostics=False,update_bootloader=0,bootloader_inputs_configured=False,filename='LXR_V2_V1.7.img',image_bytes=len(image),image_sha256=sha,magic=magic.decode(),header_hex=image[:16].hex(),payload_bytes=n,checksum=hex(checksum),payload_matches_bin=True,bin_sha256=hashlib.sha256(binary).hexdigest(),elf_sha256=hashlib.sha256((d/'LXR_V2_STM32F765.elf').read_bytes()).hexdigest(),initial_msp=hex(msp),reset_vector=hex(reset),application_origin=hex(0x08008000),highest_flash_load_exclusive=hex(0x08008000+end),flash_headroom=1015808-end,memory_section_high_water=high,sections=sections,tracked_worktree_clean=True)
(out/'verification.json').write_text(json.dumps(meta,indent=2));print(json.dumps(meta,indent=2))

```

### package.py

```python
from pathlib import Path
import base64,hashlib,io,json,subprocess,zipfile,zlib
root=Path('/workspace/lxr-desktop-app');out=Path('/tmp/phase-r2');delivery=out/'delivery';delivery.mkdir(exist_ok=True)
source='5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()==source
assert not subprocess.check_output(['git','status','--porcelain'],cwd=root)
image=(root/'build/release/LXR_V2_V1.7.img').read_bytes()
expected='00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76'
assert len(image)==288612 and hashlib.sha256(image).hexdigest()==expected
readme='''LXR FIRMWARE LAB R1 - VERIFIED TEST PACKAGE
UNOFFICIAL MODIFIED FIRMWARE - NOT A MANUFACTURER RELEASE

Intended hardware: Erica Synths LXR-02 Desktop, STM32F765VG.
NOT intended for the LXR Eurorack module or other hardware.

Source repository: https://github.com/DocLXR-02/lxr-desktop-app
Source branch: safety/sd-project-transaction-s4b2
Exact source commit: 5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527
Corresponding source and license:
https://github.com/DocLXR-02/lxr-desktop-app/tree/5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527
See LICENSE and NOTICE.md there. GPL-3.0-only with the stated terms.

Build: Release (-O3), runtime diagnostics OFF.
Toolchain: ARM GCC 14.2.1 20241119, prepared cloud toolchain.
CMake 3.31.10; Ninja 1.13.0.git.kitware.jobserver-pipe-1;
Python 3.12.14. Repository firmware version string: 1.7.

Firmware file: LXR_V2_V1.7.img
Exact file size: 288612 bytes
SHA-256:
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76

This is an application-only desktop SD update image (LXRV2IMG).
Payload starts at application flash address 0x08008000.
The bootloader binary is not included; UPDATE_BOOTLOADER=0.
No ELF, raw BIN, HEX, bootloader or full-device image is included.
The ZIP contains exactly one firmware .img file.

IMPORTANT APPROVAL AND HARDWARE STATUS
This package has NOT been tested on physical hardware.
Installation requires separate, explicit user approval for this exact
image and controlled session. Downloading or extracting it is not approval.
No device was flashed during building, verification or packaging.
Do not use or modify the user's original SD card for the experiment.
Use separate prepared test media and verified independent backups.

Recovery reference: manufacturer's public LXR-02 Desktop firmware v1.71.
https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/
The recovery image is NOT included. Obtain and verify it before approved
installation. Repository version 1.7 is not claimed equivalent to public
v1.71 or a privately supplied v1.72 test image. Recovery through the SD
updater depends on an intact compatible installed bootloader; it is not
a guaranteed recovery from every hardware/bootloader failure.

Review and first-session plan (R1):
https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-phase-r1-hardware-readiness/docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md
No installation procedure is executed or newly authorized by this package.

IPHONE DOWNLOAD AND EXTRACTION
Download LXR_LAB_R1_FIRMWARE.zip in Safari. In Files > Downloads, tap the
ZIP to extract it. The extracted folder must contain exactly:
  LXR_V2_V1.7.img
  SHA256SUMS.txt
  README.txt
Read this file before taking further action. Extracting does not install.
Keep the image filename unchanged. Verify the entire image SHA-256 with
a trusted checksum tool (or a computer) before an approved installation;
the Files app's filename/size display alone is not a checksum check.
Do not mistake the ZIP itself for an SD firmware update image.

MANIFESTS
SHA256SUMS.txt beside the downloadable ZIP checks the ZIP archive.
SHA256SUMS.txt inside the ZIP checks the firmware image and this README.
No timestamp, CPU load, audio reliability or power-loss safety is claimed
as measured on an instrument. Normal audio, Bug 001, MIDI/USB, project
save/reload and recovery behavior still require approved physical testing.
'''
readme=readme.replace('\n','\r\n').encode('ascii')
inner=(expected+'  LXR_V2_V1.7.img\n'+hashlib.sha256(readme).hexdigest()+'  README.txt\n').encode('ascii')
entries=[('LXR_V2_V1.7.img',image),('SHA256SUMS.txt',inner),('README.txt',readme)]
def make_zip():
 b=io.BytesIO()
 with zipfile.ZipFile(b,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for name,data in entries:
   i=zipfile.ZipInfo(name,(2026,10,28,0,0,0));i.create_system=0;i.external_attr=0x20;i.compress_type=zipfile.ZIP_DEFLATED
   z.writestr(i,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
 return b.getvalue()
archive=make_zip();assert archive==make_zip()
with zipfile.ZipFile(io.BytesIO(archive)) as z:
 assert z.testzip() is None and z.namelist()==[n for n,b in entries]
 assert sum(n.lower().endswith('.img') for n in z.namelist())==1
 for n,b in entries:assert z.read(n)==b
zipsha=hashlib.sha256(archive).hexdigest()
outer=(zipsha+'  LXR_LAB_R1_FIRMWARE.zip\n').encode('ascii')
(delivery/'LXR_LAB_R1_FIRMWARE.zip').write_bytes(archive)
(delivery/'SHA256SUMS.txt').write_bytes(outer)
(delivery/'README.txt').write_bytes(readme)
manifest=[]
for name in ['LXR_LAB_R1_FIRMWARE.zip','SHA256SUMS.txt','README.txt']:
 data=(delivery/name).read_bytes()
 manifest.append(dict(path='delivery/'+name,mode='100644',type='blob',bytes=len(data),sha=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest(),sha256=hashlib.sha256(data).hexdigest(),content=base64.b64encode(data).decode()))
(out/'upload-files.json').write_text(json.dumps(manifest))
meta=dict(zip_filename='LXR_LAB_R1_FIRMWARE.zip',zip_bytes=len(archive),zip_sha256=zipsha,zip_timestamp=[2026,10,28,0,0,0],zip_compression='deflate level 9',zlib_version=zlib.ZLIB_VERSION,zlib_runtime=zlib.ZLIB_RUNTIME_VERSION,deterministic_two_builds_equal=True,firmware_img_count=1,zip_entries=[dict(name=n,bytes=len(b),sha256=hashlib.sha256(b).hexdigest()) for n,b in entries],delivery_files=[{k:v for k,v in x.items() if k!='content'} for x in manifest])
(out/'package.json').write_text(json.dumps(meta,indent=2));print(json.dumps(meta,indent=2))

```

### verify_download.py

```python
from pathlib import Path
import hashlib,io,json,zipfile,subprocess
p=Path('/tmp/phase-r2');expected=json.loads((p/'package.json').read_text());result={}
for variant in ['commit','branch']:
 h=json.loads((p/f'download-{variant}-http.json').read_text());b=(p/f'downloaded-{variant}.zip').read_bytes()
 assert h['http_code']==200 and h['exitcode']==0 and int(h['size_download'])==208402
 assert len(b)==expected['zip_bytes'] and hashlib.sha256(b).hexdigest()==expected['zip_sha256']
 assert b==(p/'delivery/LXR_LAB_R1_FIRMWARE.zip').read_bytes()
 with zipfile.ZipFile(io.BytesIO(b)) as z:
  assert z.testzip() is None and z.namelist()==['LXR_V2_V1.7.img','SHA256SUMS.txt','README.txt']
  assert sum(n.lower().endswith('.img') for n in z.namelist())==1
  hashes=dict((line.split('  ',1)[1],line.split('  ',1)[0]) for line in z.read('SHA256SUMS.txt').decode().splitlines())
  for name in ['LXR_V2_V1.7.img','README.txt']:
   assert hashlib.sha256(z.read(name)).hexdigest()==hashes[name]
  image=z.read('LXR_V2_V1.7.img');assert len(image)==288612 and hashlib.sha256(image).hexdigest()=='00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76'
 result[variant]={k:h[k] for k in ['http_code','content_type','size_download','url_effective','num_redirects','ssl_verify_result']}
 assert h['ssl_verify_result']==0
assert (p/'downloaded-SHA256SUMS.txt').read_bytes()==(p/'delivery/SHA256SUMS.txt').read_bytes()
assert not subprocess.check_output(['git','status','--porcelain'],cwd='/workspace/lxr-desktop-app')
(p/'download-verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
print('PASS anonymous GitHub commit and branch ZIP downloads, exact archive bytes, CRC, both manifests and exact one approved image; checkout unchanged')

```

