# R10 startup-display correction and rebuilt delivery

Date: 2026-10-31 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.

**The corrected artifacts below supersede the earlier R10 delivery for owner use. Revision remains R10.** This is a pre-delivery display correction, not another CLP experiment. R10 remains experimental and not hardware validated; installation/listening requires separate later approval. No flashing or hardware test occurred.

## Source, CI and corrected artifacts

| Item | Identity |
| --- | --- |
| Existing source branch | `fx/clp-multivoice-max-r10` |
| Original R10 source | `1f7f4e99cf9d5d14ca859538f478ca26eb369238` |
| Corrected source | **`8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7`** |
| Correction commit | fix: simplify R10 normal startup display and document lab convention |
| Successful rebuild CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846 |
| Job | 124467300786, validate-and-build |
| Fresh image artifact | [lxr-r10-image, 13033029015](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846/artifacts/13033029015) |
| Fresh complete source/build evidence | [lxr-r10-source-build-evidence, 13032864252](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846/artifacts/13032864252) |

The source branch advanced by one normal correction commit, whose parent/merge base is exactly the original R10 head. Remote comparison shows one ahead, zero behind. No force push or new source branch was used. CI attempt 1 passed on the corrected head, including exactly one additional Release ARM build. This rebuild was expressly justified/authorized by the owner's new display request; no local ARM build or binary patch was performed.

| Corrected delivery file | Bytes | SHA-256 |
| --- | ---: | --- |
| **LXR_V2_R10.img** | **290088** | `3d8670d2d9a3448452108cc3bdd7e3cae7715aa85fc811c3a5532e11e205c119` |
| **LXR_V2_R10.zip** | **206759** | `8a8a3462277e164a6a1be3c509e7364416d0e0391fad7f67dc65e7ef17909e02` |

LXR_V2_R10.zip contains **only LXR_V2_R10.img**, byte-identical to the separately delivered image. GitHub's outer image artifact ZIP contains those two delivery files and is a different wrapper.

SOURCE.zip in the separate evidence artifact: **1754089 bytes**, SHA-256 `abd31c64a5cb90c4c325c5ef8c23ad787631db393f3acc2f3de1353cfb65ad19`.

Downloaded outer artifact wrappers match their published digests:

| Wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r10-image | 420395 | `44b339c621a70c81e7415d2e505323986440838a20c1ac52280677a9a897570e` |
| lxr-r10-source-build-evidence | 8941901 | `e5c6ca009efb0be182fcbd73711ff1e17372be5e3194dae256157d0bfee4839a` |

Artifacts were available/unexpired at verification; recorded expiry is 2026-11-30. Metadata ties both artifacts to corrected commit 8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7.

## Historical delivery is preserved

The [original R10 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r10-clp-multivoice-max-test-build/docs/codex-berichte/2026-10-31_r10-clp-multivoice-max-test-build.md), report commit **43996ab07671446f815cfc04d05a53e31092db28**, remains unchanged as historical evidence. Its earlier CI run **41176085244** and artifact identities **13032722413 / 13032752328** belong to original source 1f7f4e99cf9d5d14ca859538f478ca26eb369238. They are superseded for owner delivery by the fresh artifacts above, rather than deleted or rewritten.

Original image was 290144 bytes, SHA-256 `833a906beabff382169fa1a621a8cfb49a5de8c696d85866a0fbdb893d2856bf`; original ZIP was 206798 bytes, SHA-256 `633aa06eee836a7837bb208675c244699fb0d5ed0a77c4e29a789f268d46f26b`. Keep these distinct from the corrected files despite unchanged filenames/revision.

## Exact correction and source invariance

Read AGENTS.md and verified that both local and remote source heads were exactly the requested original R10 commit before editing. Four files change, **49 insertions / 7 deletions**:

1. **Core/Src/main.c:353–354**: normal application splash now calls lcd_clear and writes exactly `LXR LAB R10`. It removes the `-CLP` suffix, the normal second-line cursor move/comment, and the `LXR2 Drums V` / FIRMWARE_VERSION writes. No second-line text is written after clearing.
2. **AGENTS.md:12** adds the English rule: “Lab startup convention: show only `LXR LAB R<revision>` on the first line; leave the second line blank, with no effect suffix or secondary version line.” This rule is added only on the R10 candidate branch.
3. **tests/fx_clip_r10/run.py:26–74** updates only startup-specific source expectations and adds the isolated startup display check/strict correction scope guard. Existing audio assertions remain intact.
4. **tests/fx_clip_r10/package.py:12–13** requires the exact null-terminated new label and absence of the old suffix/product-line string in the image. All previous packaging assertions remain.

The corrected normal branch is:

```c
lcd_clear();
lcd_string("LXR LAB R10");
```

The second cleared line remains blank. The UPDATE_BOOTLOADER branch at main.c:343–349 still writes “Updating” and “Bootloader...” exactly as before. UPDATE_BOOTLOADER remains 0 for the application build. The test selects that branch only in an isolated native snippet; no bootloader-update firmware was built or executed.

lcd_showSplash remains at main.c:357. The later HAL_Delay(1000) and lcd_clear remain at 434–435. The LCD implementation is unchanged: lcd_clear (lcd.c:729–733) still issues the clear command and existing **10 ms** delay; lcd_string still writes the supplied characters; the currently inactive animation in lcd_showSplash is unchanged. All explicit splash/boot delays and unrelated error/UI paths are preserved. This removes display I/O, without adding new timing compensation or changing a delay.

The FIRMWARE_VERSION fallback definition at main.c:56–59 and CMake-provided definition in Core/CMakeLists.txt:35 are unchanged. Only the explicitly requested normal-splash use is removed; no other use/definition is altered.

Git tree/blob comparison confirms **all 106 production files under Core/Src/Soundengine and Core/Inc/Soundengine are byte-identical** to original R10. Overall **406 of 410 files** retain identical Git blob identities; the exact four-file change set above accounts for the remainder. The entire main.c outside the precise splash replacement matches the original source. Thus DSP, routing, fader ownership/law, ADC, storage, serialized parameters, controls and unrelated display production code are unchanged. The workflow is also unchanged.

## Actual checks and results

Final local checks and final CI passed:

- Strict source-diff guard permits only those four files, requires all audio-production bytes unchanged, and reconstructs original main.c by restoring just the removed splash text/calls. Original R9/R10 audio identity assertions remain in place.
- A native test extracts the actual normal/update preprocessor splash block and unchanged lcd_clear/lcd_string/lcd_showSplash bodies. LCD transport and time are mocked. It checks normal first-line bytes **“LXR LAB R10” plus five spaces**, **16 blank second-line cells**, one clear, no second-line cursor write and exactly 11 character writes. The update branch retains both messages, its cursor move and 21 writes. Both retain one requested 10 ms clear delay. This verifies source behavior in the model, not physical LCD transport or measured device boot duration.
- Full existing R10 native sanitizer suite: all six solo cases, 64 routing masks × six destinations, expected maximum gains, live-filter invariance, metadata/waveform 1/7/64 partitions, reset/routing lifecycle, guards/finite states, 318 unity pairs, 72 moving solo pairs and 44 principal multi-voice runs. Complete DSP history comparisons, bass/dry and VOL/D/W endpoint assertions pass. Tested idle suppression remains effective.
- Existing bug001, kit_load, project_storage and euclidean regressions pass locally and again in CI. No unrelated assertion was weakened. Storage/menu/audio callback mocks retain their previous limitations.
- Corrected CI multi-voice metrics.csv is **byte-identical to the original R10 CI metrics.csv**. Source identity and identical native metrics support preserved software behavior; they are not hardware audio validation.

Native local GCC is Debian 14.2.0; CI native GCC is Ubuntu 13.3.0; NumPy 2.3.5. Native flags retain ASan/UBSan/bounds/float-cast-overflow with nonrecovering failures. The final committed suite runs in CI through the unchanged workflow.

## One additional Release ARM build and resources

Established compiler: **Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), arm-none-eabi-gcc 14.3.1 20250623**. Version/settings match original R10. Toolchain archive SHA-256 `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd` matches original evidence; the workflow records the downloaded digest rather than enforcing an independently supplied expected archive digest.

| Region | Original R10 | Corrected R10 | Delta |
| --- | ---: | ---: | ---: |
| ITCM / 16384 B | 15608 | **15608** | 0 |
| DTCM / 131072 B | 107672 | **107672** | 0 |
| RAM / 393216 B | 274648 | **274648** | 0 |
| Linker FLASH / 1015808 B | 237524 | **237468** | **−56** |
| Image | 290144 | **290088** | **−56** |

.text=153504 B (−24), .rodata=82584 B (−32). .dtcm=36996, .dtcmz=70672 and .itcm_text=15608 are unchanged. ITCM free space remains 776 bytes. No placement, diagnostics or DSP optimization was changed. The original R10 processing/stack/timing uncertainties remain; memory fit and native results do not prove device deadline capacity. No hardware performance measurement occurred.

## Rebuilt image and downloaded-artifact verification

CI packaging and independent validation of downloaded bytes passed:

- LXRV2IMG magic; payload **290072 bytes** equals file length minus 16-byte header.
- Internal checksum **0xBD**, independently recomputed; three reserved bytes zero.
- Exact null-terminated **LXR LAB R10** present; **LXR LAB R10-CLP** and **LXR2 Drums V** absent.
- SP **0x20080000**, 8-byte aligned/in configured SRAM boundary; reset **0x08013555**, Thumb bit set/in application load bytes, matching ELF entry point.
- Four nonempty physical ELF LOAD ranges, exclusive ends: **0x08008000–0x08041c48**, **0x08041c48–0x08041f9c**, **0x08041f9c–0x0804b020**, **0x0804b020–0x0804ed18**. All are application-only below 0x08100000, with no bootloader load bytes.
- Those ranges cover the whole payload contiguously; every loaded ELF byte matches its image byte. Zero-file-size BSS/heap segments are not image payload.
- ZIP integrity, exact single-image membership and contained/separate image identity pass.
- Image/ZIP/source hashes match CI package output, SHA256SUMS and artifact metadata as applicable. Both outer wrapper digests match published GitHub values after download.
- SOURCE_COMMIT.txt identifies corrected source. All **410 archived source blobs** and the exact file-member set match its Git tree. Source archive integrity/size/hash pass. Host result source hashes/reference fields match delivered unchanged audio sources and pinned R9 reference.

Evidence contains exact source/SOURCE.zip, ELF/map, compile_commands.json, native/ARM versions, toolchain archive hash, build/test logs, results/CSV/WAVs, disassembly, section/load reports, validation JSON and checksums. These checks establish correspondence and download integrity, not a second reproduced ARM binary or physical display/audio validation.

## Commands and failed preliminary attempts

Actual final commands included:

```sh
git ls-remote --heads origin
git diff 1f7f4e99cf9d5d14ca859538f478ca26eb369238 --exit-code -- Core/Src/Soundengine Core/Inc/Soundengine
python3 -m py_compile tests/fx_clip_r10/run.py tests/fx_clip_r10/package.py
python3 tests/fx_clip_r10/run.py --output build/r10-host
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
git -c core.whitespace=cr-at-eol diff --cached --check
git push origin fx/clp-multivoice-max-r10
```

CI repeats the final suites, then executes cmake --preset release; cmake --build --preset release --parallel 2; python3 tests/fx_clip_r10/package.py; section/load/disassembly/version recording; and git archive of the corrected HEAD. Exactly one additional Release build ran, on CI attempt 1. No earlier binary was patched or repackaged as the correction.

Downloaded audit:

```sh
unzip -q /workspace/attachments/f883df3f-9e61-4e78-9f84-8cc2540b8694/lxr-r10-corrected-image.zip -d /workspace/r10-display-downloads/image
unzip -q /workspace/attachments/65547d33-e1c4-4128-86cf-59704d0f0d51/lxr-r10-corrected-evidence.zip -d /workspace/r10-display-downloads/evidence
python3 /workspace/r10-display-evidence/verify-download.py
```

The temporary verifier checks all published hashes, archive members, Git blobs, headers/vectors/splash, application boundaries and ELF/image bytes. Its successful JSON is /workspace/r10-display-evidence/download-verification.json.

Preliminary failures are recorded rather than treated as candidate defects:

- An exploratory read used nonexistent IO/lcd paths; the implementation is Core/Src/lcd.c / Core/Inc/lcd.h.
- A quoting error in a temporary edit script prevented it from applying changes. Subsequent shell commands still launched the old host suite; edits then overlapped that run. Its unchanged source-stability guard correctly rejected the run. Edits were completed and tests rerun without concurrent changes; no guard was disabled. Log: /workspace/r10-display-evidence/pre-edit-run.log.
- The new native display extraction initially selected the first UPDATE_BOOTLOADER block in main.c (the includes section), causing a missing usb_device.h compile error. Extraction was narrowed to the last such block before lcd_showSplash. Log: splash-extraction-attempt.log. The final test checks both intended startup branches.

Final host/regression logs are under /workspace/r10-display-evidence; CI evidence archives the final committed checks/build. No final assertion, sanitizer, regression, ARM build, packaging or downloaded-byte verification failure remains. No retry ARM build, force push or repository-setting change was used.

## Publication and remaining approval

Only the authorized R10 source branch advances. Every other pre-existing remote head, including main, lab-setup and the original R10 report, is preserved. This new report-only branch is based exactly on **5829398cea5d337200393757e39a37d4d8148256**, and its diff contains only this Markdown report.

Report branch: **codex-reports/2026-10-31-r10-startup-display-correction**.
Report path: **docs/codex-berichte/2026-10-31_r10-startup-display-correction.md**.

R9's previously owner-approved sound/solo response remains historical hardware evidence. R10's Maximum policy, including silent high faders holding the shared return open, is unchanged software and remains pending separate installation/listening approval. This display correction is not policy acceptance, a frozen stage, a release or authorization to flash. No merge, release, setting change, flashing or hardware test occurred. Remote source/report contents and exact report URL are verified after publishing.
