# LXR-BUG-030 — Reject invalid MIDI CC0 Kit indices (implementation checkpoint)

**Task date:** 2026-11-23
**Execution:** ChatGPT, authorized bounded firmware source change; no Codex/WORK delegation
**Status:** SOURCE CHANGED AND REMOTE CODE DIFF VERIFIED; **NOT BUILT / NOT HOST-TESTED / NOT ARM-TESTED / NOT DEVICE-TESTED / NOT RELEASED**
**Firmware repository:** `DocLXR-02/lxr-desktop-app`
**Stable/accepted source base:** [R22 TEST03 `0e37942250f79306627702061d851731bdfdbbc8`](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8)
**Firmware candidate branch:** `fix/cc0-reject-invalid-kit-r22-20261123`
**Firmware candidate head:** [`280d57972a03bf65e3e0e36145abd4fa082a3d0b`](https://github.com/DocLXR-02/lxr-desktop-app/commit/280d57972a03bf65e3e0e36145abd4fa082a3d0b)
**Independent documentation branch:** `docs/cc0-reject-implementation-report-20261123`, based on `lab-setup` head `5829398cea5d337200393757e39a37d4d8148256`; the report branch contains **no firmware source changes**.
**Lab governance actually read:** private Lab-main `fd2899acdee011bbff6ead909710f324595c1f1a`, `README.md` and `WORKFLOW.md`; earlier [CC0 risk analysis](https://github.com/DocLXR-02/lxr-firmware-lab/blob/b1f4866d04e5b3057f96629da2c7b4445fdea25c/cases/LXR-BUG-030-CC0-KIT-BOUNDS.md) and [safety-first rule](https://github.com/DocLXR-02/lxr-firmware-lab/blob/b1f4866d04e5b3057f96629da2c7b4445fdea25c/WORKFLOW.md) were verified in Lab staging.

## Owner decision and authorization

The owner confirmed 64 Kits remain and approved **policy B** for implementation: accept MIDI CC0 values 0–63 and reject 64–127 before changing selected Kit, audio Kit, UI state or Kit autosave. Preserve the existing independent `preset_loadDrumsetRam()` guard, accepted R22 and musically approved R18. Owner separately authorized **Lab documentation promotion to Lab-main**, but **did not authorize host tests, ARM builds, CI execution, device actions, flashing or firmware-main merge**.

## Evidence and technical rationale

- [R22 `Core/Src/frontPanelParser.c:103–116`](https://github.com/DocLXR-02/lxr-desktop-app/blob/0e37942250f79306627702061d851731bdfdbbc8/Core/Src/frontPanelParser.c#L103-L116) previously copied `frontParser_midiMsg.data2` into `menu_selectedPresetNr[0]` without validating the 0–63 bound, before `preset_loadDrumsetRam()`, dirty timer and LCD repaint.
- `NUM_KIT_SLOTS = 64`, [`Core/Inc/Parameters.h`](https://github.com/DocLXR-02/lxr-desktop-app/blob/0e37942250f79306627702061d851731bdfdbbc8/Core/Inc/Parameters.h#L22).
- The accepted R22 Kit loader has its own `if(presetNr >= NUM_KIT_SLOTS) return;` [guard](https://github.com/DocLXR-02/lxr-desktop-app/blob/0e37942250f79306627702061d851731bdfdbbc8/Core/Src/Preset/presetManager.c#L815-L819); it protects its array access, but not all UI-selected Kit-name lookups.
- Lab [CC0 risk decision record](https://github.com/DocLXR-02/lxr-firmware-lab/blob/b1f4866d04e5b3057f96629da2c7b4445fdea25c/cases/LXR-BUG-030-CC0-KIT-BOUNDS.md) establishes that UI Save/Load name-table accesses can be out of bounds following invalid CC0. A device crash/corruption was **not** demonstrated.
- Erica's later public source uses `value % 64`, but the owner expressly selected **reject** to preserve existing behavior and to avoid unnecessary full Kit loads on invalid CC0.

## Minimal source change (committed)

Exactly one production file was modified, **`Core/Src/frontPanelParser.c`**, in the CC0 handler only:

```c
if(parNr == 255/*PAR_BANK_CHANGE*/)
{
    const uint8_t kitNr = frontParser_midiMsg.data2;
    if(kitNr >= NUM_KIT_SLOTS) return;
    menu_selectedPresetNr[0] = kitNr;
    //preset_loadDrumset(kitNr, 0);
    preset_loadDrumsetRam(kitNr,0, 0);
    preset_touchTimer(PRESET_KIT_TIMER);
    menu_repaint();
    return;
}
```

The local `uint8_t` captures volatile MIDI `data2` once instead of twice; the guard runs before selected Kit mutation. Normal 0–63 action order remains. Other CC/NRPN branches, sound-engine, Kit loader, presets, project format and audio DSP are untouched. Exact [candidate source](https://github.com/DocLXR-02/lxr-desktop-app/blob/280d57972a03bf65e3e0e36145abd4fa082a3d0b/Core/Src/frontPanelParser.c).

## Executed versus unexecuted evidence

**Executed (read-only verification and authorized GitHub write):**
- Retrieved exact accepted R22 source and relevant header; re-read target code from isolated firmware branch before replacing one exact source span.
- Committed the single-file source patch through GitHub contents API. Commit `280d57972a03bf65e3e0e36145abd4fa082a3d0b`.
- Independently fetched patched file at that commit and confirmed the new guard occurs before `menu_selectedPresetNr[0]` and the loader/timer/repaint calls.
- GitHub comparison against pinned R22 showed **one modified file**, 5 added and 3 removed lines; one commit ahead, zero behind.

**NOT EXECUTED:** no compiler, host unit test, sanitizer, disassembly/size comparison, ARM target build, static runtime measurement, GitHub Actions run, firmware IMG generation, flash, listening test or device operation. No tests are claimed PASS. No CPU, flash or memory savings are measured. The added source is a candidate only.

## Later independently approved verification gates (not authorized by this task)

1. Isolated host/ASan behavior checks for CC0 values 0–127, especially the invalid half range (64–127) and UI Save/Load Kit-name paths. Confirm invalid events do not mutate selected/loaded Kit, active parameters, display state, or dirty timer; valid 0–63 retain previous behavior. Include a negative oracle that catches the original R22 defect.
2. Under separate explicit approval, compile with the project's defined ARM toolchain; compare code size and diagnostics with pinned accepted R22 and preserve original evidence.
3. Only after the device returns, approval, safety backup and a recovery procedure: verify normal legitimate MIDI Kit selection and Save/Load operations on an expendable test project. Do NOT intentionally drive the vulnerable accepted R22 UI with invalid CC0 values; never risk user projects.
4. Document owner approval separately from host and CI evidence before calling the fix device-tested or integrating into any stable/released firmware.

## Delivery and restrictions

**Firmware branch committed/pushed, but intentionally not merged.** R22 TEST03 remains accepted stable; R18 CLP remains the accepted sound reference. No firmware-main merge or release, no device operations. The Lab documentation synchronization is a separate authorized action; see its own commit/PR details in Lab records when available.

**Task complete at source-only checkpoint.** No additional bug scope is implied.
