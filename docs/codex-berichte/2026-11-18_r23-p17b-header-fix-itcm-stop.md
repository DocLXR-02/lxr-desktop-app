# R23 P17-B — Corrected FX prototype; ARM ITCM link STOP

**Date:** 2026-11-18 (UTC)  
**Task:** Owner-authorized one-declaration correction in R23 and rerun of the existing host/ARM CI workflow.  
**Disposition:** **STOP — HOST PASS / ARM LINK FAIL (ITCM overflow by 32 bytes).**  
**Source repository:** `DocLXR-02/lxr-desktop-app`  
**Protected accepted firmware:** R22 TEST03, `0e37942250f79306627702061d851731bdfdbbc8`.  
**Development branch:** `release/r23`; final verified head **`d796b7da6d4f885d01895f6dec95eeb3b73e64d2`**.  
**Documentation base:** `docs/lab-project-state` at `672891c23809e8a2014457f564c16732f970e4f9`.  
**Report branch:** `reports/r23-p17b-itcm-stop-2026-11-18` (documentation only).

## Scope and permission

The owner specifically authorized fixing the previously identified incomplete declaration of `FxBus_processSelected()` and rerunning the same CI tests. No permission was granted to alter the DSP algorithm, adjust memory placement, rewrite or broaden firmware, change the UI or kit format, flash hardware, merge, release or declare audio acceptance. CHAT and existing GitHub Actions were used; no Codex or WORK.

The previous [P17-B validation STOP report](https://github.com/DocLXR-02/lxr-desktop-app/blob/98310a69e72ae829424cbf0be9743f78aa1e6023/docs/codex-berichte/2026-11-18_r23-p17b-validation-stop.md) records the original host PASS / ARM syntax failure and its reusable evidence.

## Exact changes

| Commit | Scope | Finding |
|---|---|---|
| [`be469f66a68ed8bcc2add2ee314040fa12bce64c`](https://github.com/DocLXR-02/lxr-desktop-app/commit/be469f66a68ed8bcc2add2ee314040fa12bce64c) | Only `Core/Inc/Soundengine/FxBus.h`; completed the new seven-argument prototype | GitHub diff verified: one line changed |
| [`d796b7da6d4f885d01895f6dec95eeb3b73e64d2`](https://github.com/DocLXR-02/lxr-desktop-app/commit/d796b7da6d4f885d01895f6dec95eeb3b73e64d2) | Only the new declaration's CRLF-to-LF line ending | Corrects the existing `git diff --check` whitespace gate without changing the declaration text |

Correct declaration:

```c
void FxBus_processSelected(uint8_t fxType, int16_t *fxBuffer,
                           int16_t* outL,int16_t* outR,
                           int16_t* outL2, int16_t* outR2,
                           uint32_t size);
```

The actual source declaration occupies one line; the multiline formatting above is for readability. Its types and order were checked against the unchanged function definition in `Core/Src/Soundengine/FxBus.c`.

The comparison from pre-correction validation SHA `8a31e690b28338bf00c66d382bb62afc10076e18` to the final head includes **only `Core/Inc/Soundengine/FxBus.h`**. The existing CI workflow `.github/workflows/r23-p17b-validation.yml`, DSP sources, routing logic, tests, UI, preset logic and bootloader were unchanged in this task. R22 and `main` remained at their original source SHAs.

## CI attempts (all failures retained)

1. **Inherited initial validation:** [Run 44233493705](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705), source `8a31e690b28338bf00c66d382bb62afc10076e18`: host PASS; ARM compiler FAIL because the newly declared function prototype was incomplete. See previous STOP report and raw CI logs.
2. **First approved correction:** [Run 44234325498](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234325498), source `be469f66a68ed8bcc2add2ee314040fa12bce64c`: both jobs STOP before tests because `git diff --check HEAD~1 HEAD` reported `Core/Inc/Soundengine/FxBus.h:66: trailing whitespace`. The added line inherited the file's CRLF terminator. No test or ARM compile results should be attributed to this attempt.
3. **Declaration format corrected:** [Run 44234450455](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455), source `d796b7da6d4f885d01895f6dec95eeb3b73e64d2`: host job **PASS**; ARM source compilation reached link stage, then linker **FAIL** due to ITCM capacity.

### Final run 44234450455 — host result

Job [134567073561](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455/job/134567073561), conclusion **success**. Exact `tests/fx_pool_r23/run.py` output confirms **280,032 route/mask cases PASS**. Existing R18 Delay mono (144 cases), Compressor mono (including normal-domain 144 cases), CLP kernel comparison (135 state snapshots), USB-MIDI, R21/R22 voice copy and Kit/Project storage/transaction regressions ran successfully. These are host tests; they do not prove complete R23 mixed-FX audio behavior or real-device timing.

### Final run 44234450455 — ARM result

Job [134567073106](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455/job/134567073106), conclusion **failure**. Arm GNU 14.3.rel1 toolchain archive passed its SHA-256 check; CMake configured; the target source objects compiled and the failure occurred during ELF linking.

Exact diagnostic excerpts:

```text
ld: LXR_V2_STM32F765.elf section '.itcm_text' will not fit in region 'ITCM_RAM'
ld: region 'ITCM_RAM' overflowed by 32 bytes
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       16416 B        16 KB    100.20%
            DTCM:      109216 B       128 KB     83.33%
             RAM:      275164 B       384 KB     69.98%
           FLASH:      245724 B       992 KB     24.19%
collect2: error: ld returned 1 exit status
```

Reported R22 static resource baseline (from the accepted resource ledger): ITCM **15,304 B**, DTCM **107,424 B**, general RAM **275,164 B**. Against those recorded baselines, the failed R23 link reports **ITCM +1,112 B**, **DTCM +1,792 B**, RAM unchanged. These are link-stage memory-usage figures, **not measurements of CPU load or live stack**. The new DTCM allocation matches the prototype's planned fixed FX input and return workspace. The ITCM excess requires a future separately approved resource optimization; freeing only the 32-byte minimum would leave no practical headroom and should not be treated as sufficient engineering margin.

**Not produced / not executed:** successful ELF, IMG, MAP, firmware image checksum, finished post-build section comparison, physical device test, full combined-FX audio regression, CPU/WCET, IRQ/DMA deadline measurement, stack high-water or long-term reliability. No firmware image is approved for flashing. The CI artifact holds the recorded failed-link log and earlier job evidence.

## Raw evidence and retention

- [Final host artifact 14397145551](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455/artifacts/14397145551): SHA-256 `11d6d34158c8db7db554de3e95fee6d6190ee1faede9a76ce5851a29c5049a5a`, 6,127 bytes, expires 2026-12-18.
- [Final ARM failure artifact 14395854910](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455/artifacts/14395854910): SHA-256 `171084a71c2a8bfba5f3cf83bb700f1affa2936cbd0035adc589d049ec286d57`, 4,821 bytes, expires 2026-12-18.
- [Formatting-failure CI run 44234325498](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234325498): both jobs stopped by the whitespace check, with job logs and uploaded small failure evidence.
- The CI YAML at [pinned source](https://github.com/DocLXR-02/lxr-desktop-app/blob/d796b7da6d4f885d01895f6dec95eeb3b73e64d2/.github/workflows/r23-p17b-validation.yml) retains the exact executed commands. Raw GitHub job logs are available through their links. CI artifact retention is finite; no claim of permanent archive is made.

## Disposition and next authorization gate

**STOP.** Do not modify ITCM annotations, linker settings, DSP functions, effect routing, test expectations or the UI without additional, narrow approval. The next candidate task would be a measured, small-scope R23 ITCM reduction, preserving the one-effect path and all R22 DSP algorithms. Confirm the source SHA, inspect the assembly/map and instrumented size evidence, and rerun the mandatory Host/ARM CI. Do not cut audio or memory-safety tests to obtain a green build. A successful future build still does not establish hardware timing or sonic acceptance.

No protected branches were modified; no merge, release, hardware test or flash was attempted. **Current development head: `d796b7da6d4f885d01895f6dec95eeb3b73e64d2`**, experimental and NOT FOR FLASHING.
