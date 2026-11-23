# LXR-BUG-030 — CC0 reject policy: executed host sanitizers and matched ARM resource comparison

**Date:** 2026-11-23  
**Task:** Owner explicitly authorized HOST/memory-safety tests and ARM build/resource comparison, following the previously approved policy-B source patch.  
**Status:** **HOST PASS / ARM BUILD PASS / STATIC RESOURCE CHECK PASS / DEVICE NOT RUN**  
**Firmware repository:** `DocLXR-02/lxr-desktop-app`  
**Source base (accepted stable):** [R22 TEST03 `0e37942250f79306627702061d851731bdfdbbc8`](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8)  
**Source-only patch:** [`280d57972a03bf65e3e0e36145abd4fa082a3d0b`](https://github.com/DocLXR-02/lxr-desktop-app/commit/280d57972a03bf65e3e0e36145abd4fa082a3d0b), one production source file  
**Validation source branch:** `fix/cc0-reject-invalid-kit-r22-20261123`; PR head [`649c649df43c7a717d59edc28254d06e702ad6b3`](https://github.com/DocLXR-02/lxr-desktop-app/commit/649c649df43c7a717d59edc28254d06e702ad6b3)  
**GitHub PR event merge checkout:** `180d556f6467cc07b47313d0ad51940fbb06446d`; synthetic PR merge commit, base remains `release/r22` at the exact accepted R22 SHA.  
**Draft validation PR:** [#3 — VALIDATION ONLY](https://github.com/DocLXR-02/lxr-desktop-app/pull/3), **DO NOT MERGE**  
**CI run:** [45216186960](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45216186960), GitHub Actions workflow `cc0-r22-reject-validation.yml`, run number 3, **conclusion: SUCCESS**, both jobs completed SUCCESS.  
**Lab governance actually read:** `DocLXR-02/lxr-firmware-lab` main commit `7c0d740501d6e3a4f0501113475a7f51fdc25b41` (README, WORKFLOW, CC0 record, bug register, evidence).

## Executed host and memory-safety verification

**Job:** `CC0 real-handler ASan/UBSan + R22 negative oracle`, job ID `137837848394`, **SUCCESS**.

- `tests/cc0_bounds/run.py` extracts the real `frontPanel_ccHandler()` body from `Core/Src/frontPanelParser.c`, plus the real `copyNameToEditBuffer()` and `copyEditBufferToName()` functions from the menu source; compiles a bounded host harness with `-fsanitize=address,undefined,bounds`. The surrounding full target firmware and interrupts are mocked; this is not complete firmware execution.
- **64/64 valid CC0 values (0–63) PASS:** exactly one Kit load, Kit selected/loaded state consistent, once each for touch timer and repaint.
- **64/64 invalid CC0 values (64–127) PASS:** no Kit selection/loaded Kit mutation, no Kit load, no dirty-timer or repaint side effects; no resulting invalid Kit-name-array access in the instrumented test path.
- **Adjacent regular CC PASS:** ordinary CC handling remains functional.
- **Accepted R22 negative control EXPECTED FAILURE:** the historical handler reaches an invalid selected Kit and the instrumented Kit-name lookup signals the memory-safety defect. The job treats this deliberately failing control as a successful detection, not as an arbitrary test failure.
- `python3 tests/r22_drum_copy/run.py` **PASS**, including production Kit/voice-copy transaction and project-storage regressions, encoder/selected Kit controls, and a documented 448-case Kit label/index oracle.
- `python3 tests/r18_usb_midi/run.py` **PASS**, including expected reproduction of historical original USB failures and successful accepted-candidate USB-MIDI cases. The historical expected-fail cases are not regressions in this CC0 candidate.

**Original uploaded host evidence:** [artifact `14839415697`](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45216186960/artifacts/14839415697), 4,418 bytes compressed, ZIP SHA-256 `2d8acd81c74a2f877f8bdf61cf184a916b879a1a4486d46e73df64b94910ead1`. Includes compiled-run logs, negative control result, summary JSON and preexisting regression logs. GitHub reports expiry **2026-12-23**.

## Executed ARM builds and exactly matched static resource comparison

**Job:** `ARM GNU 14.3 R22 vs CC0 source candidate, static resources`, job ID `137837847927`, **SUCCESS**.

The workflow downloads ARM GNU **14.3.rel1** with a pinned SHA-256 and compiles the accepted R22 source and candidate in separate clean Release CMake/Ninja build directories with identical compiler/version and options. Two ELF/IMG images were built without flashing or invoking device operations.

| Section/region | R22 accepted source fresh build | CC0 candidate fresh build | Delta |
|---|---:|---:|---:|
| `.text` bytes | 160,712 | 160,720 | **+8** |
| `.rodata` bytes | 83,272 | 83,272 | 0 |
| ITCM occupied span | 15,304 | 15,304 | 0 |
| DTCM occupied span | 107,424 | 107,424 | 0 |
| SRAM occupied span | 275,164 | 275,164 | 0 |
| ELF-linked firmware image payload | 297,664 | 297,672 | **+8** |
| Full `.img` (16-byte header included) | 297,680 | 297,688 | **+8** |

**Candidate static capacity margins, based on existing linker/validation budgets:**

- ITCM: `16,384 − 15,304 = 1,080 B`
- DTCM: `131,072 − 107,424 = 23,648 B`
- SRAM: `393,216 − 275,164 = 118,052 B`
- Firmware payload region: `1,015,808 − 297,672 = 718,136 B`

Both ARM ELF architecture checks, LXR IMG magic/payload length/checksum, and section/region capacity checks **PASS**. Static resource numbers come from the newly generated matched builds, not guessed from historical reports. The newly built baseline IMG has the same **size** as accepted R22 TEST03; byte-identical SHA-256 comparison to the originally accepted distribution was **not independently verified in this task**. The candidate IMG is **not an owner-approved update**.

**Original uploaded ARM evidence:** [artifact `14838426863`](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45216186960/artifacts/14838426863), 1,649,859 bytes ZIP, SHA-256 `b4ac699710a97146c51a16a8374f2e7971eb8ad63d827b42ef7e59a27222160e`. Includes both .elf/.map/.img sets, build/configuration and section logs, candidate disassembly, source SHA records, `resource-comparison.json`, per-file SHA-256 checksums and NOT_FOR_FLASHING note. GitHub reports expiry **2026-12-23**.

## Evidence limits and engineering conclusion

**Confirmed by executed checks:** on the extracted production CC0 path, policy B accepts 0–63, rejects 64–127 without state or timer changes, and protects the actual Kit-name array functions from CC0-origin invalid indices; original R22 negative control detects the missing guard. Existing bounded host regressions pass. Both compared ARM builds link and the entire source change increases `.text` and IMG size by **only 8 bytes**, with **no delta in static ITCM/DTCM/SRAM**.

**NOT measured:** CPU cycles, execution-time distribution/WCET, DMA/IRQ deadline margins, run-time stack high-water, runtime heap, long-term reliability, stress on a physical MIDI/USB connection, hardware audio and owner listening acceptance. One tiny condition is an *event-path* operation, not a per-sample DSP step. Avoid unsupported claims of a quantified CPU optimization.

**Preserve:** accepted R22 TEST03 as stable fallback; accepted R18 CLP sound reference. Kit count is 64, unchanged. Preserve the separate `preset_loadDrumsetRam()` guard, project formats and all DSP.

**Further authorization required:** Device availability, recovery backup, safe legitimate-value Kit/Save/Load checks, owner acceptance and **any** merge into firmware `release/r22` or `main`. The PR remains DRAFT and should not be treated as a release candidate. Artifact evidence should be durably copied to the owner's existing archive before its scheduled 30-day expiry, under any applicable archive transfer approval.

**Changes this validation task made:** two test scripts (`tests/cc0_bounds/run.py`, `resources.py`) and one validation-only CI workflow on the isolated source candidate branch; a draft PR for visibility and traceability; and this report on an independent documentation-only branch. **No further production code changes after the original CC0 patch.** Both CI jobs were actually executed and completed successfully.
