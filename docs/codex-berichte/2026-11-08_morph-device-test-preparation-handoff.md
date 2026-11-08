# Kit Morph / Filter Drive A–B–A hardware test — preparation handoff

Date: 2026-11-08. This document is a **test preparation record**, not a device observation or release approval.

## Objective

Obtain comparative evidence on a physical LXR-02 for the previously owner-observed Filter Drive discontinuity during kit Morph, starting from official v1.71, comparing the isolated reserved-command guard, and repeating v1.71 as a control. Reuse a frozen disposable test project with identical kit pairs, keep monitor levels low, and record the actual physical results.

## Controlled images

1. A1/A2 manufacturer original v1.71 from user-supplied bytes. SHA-256 `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576`. Original file name `LXRV2_update_v1.71(1).img`, 273,544 B.
2. B experimental source-pinned `c433f0ecc68ba66dc885784ad03905c8be18cf39`, output `LXR_V2_V1.7.img`, SHA-256 `b503f43a22201cc0eac1fdb3a6cb98f75d15e66a849615ab154f3692152a3143`, 274,736 B. Same binary as [successful matched ARM CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268).
3. B's own clean build [CI 42532306552](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42532306552) passed native regressions and IMG integrity; see separate verified report under this branch.

The two images have verified contents, but **no on-device installation has been attempted by ChatGPT**. Never present B as official production v1.71; the GitHub source/official-binary correspondence is unproven.

## On-device protocol summary

- Ensure two verified backups of current project data, reliable return to the owner's preferred accepted firmware (including R19 where applicable), and a spare FAT32 test SD card; do not risk working performance data.
- Install A1 by normal SD-card updater: one correctly named `LXR*.img` at a time; hold encoder at power-on, observe successful checksum/update and boot; never interrupt the update.
- Build disposable test kits on A1, with exact matched source/Morph-target pairs for each Drive/filter setting. Save the complete test project, remove all IMG files from SD root, freeze a full read-only copy. Record baseline.
- Test identical-kit LP Drive 0/32/64/96/127, filter types BP/HP/OFF, slow and rapid Morph sweeps, endpoint rotations, stopped/manual trigger and 160 BPM programmed kick. Check Voice 1 first and optionally 2–6. Always restore the same frozen fixture between stages.
- Install B and repeat exact safe conditions, recording both corrected behavior and genuine Morph responsiveness; check loading kit and Morph target.
- If safe, restore A2 original v1.71 and repeat the decisive baseline reproduction. Optional final restoration of R19 is a separate documented hardware operation.
- Score 0–3 click severity per take, note any unintended Drive timbre reset and screen messages, retain comparable audio captures.
- Stop on strong unexpected output, sustained noise, crash, unexpected storage writes/compatibility problems, power/update errors or unverified backup. No external MIDI, unsupported NRPN, FX-CLP, deliberate Euclidean length-16 or timing-probe campaign.

The complete executable-owner procedure, CSV and template are in owner-downloaded `LXR_MORPH_Device_Test_Plan.zip`, SHA-256 `c0791149135a97a6edcf8493e0a8127127f839cb558807569db5d52c9b8d9ba1`, containing English `DEVICE_TEST_PLAN.md`, `TEST_MATRIX.csv`, `SESSION_LOG_TEMPLATE.md`, `IMAGE_MANIFEST.json` and `README_FIRST.txt`. This public record is an index; the referenced ZIP is retained separately in the conversation.

## Exclusions and unresolved proof

No hardware validation, firmware source edit, release, main/R4/R11/R18/R19 modification, bootloader action, manufacturer PR or merge. No confirmation of official v1.71 binary equivalence, measured CPU load/WCET, audio DMA/IRQ deadline, live stack margin or long-term stability. The owner should provide observations before any upstream PR is drafted or submitted.

## Sources

- Manufacturer manual pp. 20–21, 39 and [manufacturer v1.71 listing](https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/).
- [Manufacturer source and installer guidance](https://github.com/erica-synths/lxr-desktop-app).
- [Isolated Morph patch](https://github.com/DocLXR-02/lxr-desktop-app/commit/c433f0ecc68ba66dc885784ad03905c8be18cf39).
- [Existing host regression CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42530896384).
- [Matched build comparison CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268).
- [Physical test image build CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42532306552).
