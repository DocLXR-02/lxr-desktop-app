# BUG008 — Owner Authorization for Limited Device Installation and Test

**Date:** 2026-11-10 (Europe/Vienna)  
**Status:** **AUTHORIZED, PACKAGED, NOT YET DEVICE-TESTED.** This is an authorization record, **not** a hardware observation or release certificate.

## Decision and boundaries

Following successful R19-derived BUG008 ARM/host validation, the owner explicitly approved installing and testing the **existing experimental candidate** on their own LXR-02. The assistant has no remote access to the instrument and has not flashed, installed or measured any device.

Authorization covers one bounded real-device evaluation of the unchanged candidate image only. It does not authorize a production release, a new firmware version label, code edits, a merge, changes to accepted R19/R18 references, hardware modification, bootloader flashing, or sound alterations. Any unexpected feedback, audio failure, update failure or loss of normal MIDI-note recording is a STOP condition. Retain the approved R19 recovery image and a full SD-card/project backup.

## Exact candidate provenance

- Accepted R19 source: `638b6eaae5b1624637d20cbc11a4a3e0175edca2`.
- Experimental candidate branch: `fix/bug008-continuous-automation-r19-2026-11-10`.
- Exact successfully built candidate SHA: `2e3a60e251dffbceefc0ee4af258463f5d87126c`.
- [Final successful CI run 42842043263](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42842043263).
- [Separate per-build ARM/host report](../codex-berichte/2026-11-10_bug008-continuous-loop-automation-candidate-arm-validation.md).
- Original verified image from CI: `BUG008_TEST_NOT_FOR_FLASH.img`, 290,232 bytes, SHA-256 `6f5e5d2e6dd192b9385ddd0b8a95e14f4c06234b3de4f4e05f1a3878816c6f30`.
- **Owner test delivery:** `LXR_V2_BUG008_TEST.zip` (207,559 bytes; SHA-256 `e03f9e0cb5a6ba011f17043422ec6d343856df761b16ba6e37733db81f0e747d`), containing exactly one file: `LXR_V2_BUG008_TEST.img`. The image is byte-for-byte identical to the successful CI image. ZIP CRC and member SHA-256 checked; the image update header, payload length and internal additive checksum were also verified independently. There was **no new ARM build**.
- Separate English documentation ZIP: `LXR_BUG008_Device_Test_Documents.zip` (5,083 bytes; SHA-256 `2b1fd6655973dd7f6b8e0150e5482a2c1e3f18187dd62d3217bad0d363b7e012`), containing a hardware-test checklist and packaging record. These conversation artifacts are **not claimed to be durably archived in Library**.
- The firmware startup screen intentionally still says **`LXR LAB R19`**. Distinguish the candidate by `SHIFT+REC`: `mod=org/par`. This is a validation build, not an accepted R20.

## Test intent and expected observations

The owner should use a disposable 16-step pattern (ideally all note steps active initially), conservative output levels, selected automation lane 1, and `mod=par`. A complete changed loop should replace the selected automation lane coherently; an untouched following loop should not overwrite the last completed take; another complete take should replace the prior one; the other lane should be preserved. Verify `mod=org` still records ordinary MIDI notes. Check one-loop start alignment, physical knob input, target switching, and a manual abort.

**Known deviation from the earlier design discussion:** Early REC-off or transport stop **discards** the incomplete take instead of filling the unwritten tail. This is an explicit open implementation limitation, not a hardware failure. The stock step-automation playback path still ignores inactive steps; the recording change does not make such steps audible.

## Status and next gate

**At the time of this record, owner installation and all real-device results are UNKNOWN / NOT YET REPORTED.** Hardware sound behavior, external MIDI behavior, physical input reliability, CPU/WCET, IRQ/DMA deadlines, stack high-water and sustained reliability remain unmeasured or untested. Await the owner's actual test observations before an acceptance decision or another build. The accepted R19 firmware and R18 sonic reference remain protected.
