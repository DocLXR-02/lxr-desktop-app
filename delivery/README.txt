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
