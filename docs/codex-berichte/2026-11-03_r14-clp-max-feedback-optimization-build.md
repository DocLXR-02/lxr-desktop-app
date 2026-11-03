# R14 — Shared CLP MAX-feedback control and mixer ITCM optimization

Date: 2026-11-03 (Europe/Vienna). **Non-Codex Chat/GitHub Actions implementation and candidate build**. No release, merge, instrument flash, or hardware test. The owner authorized this single bounded R14 task after hearing and accepting the R13 solo-feedback behavior.

## Pinned identity and comparison

| Item | Exact identity |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Tested R13 source baseline | `149320f082f79edcb6a029f039fdb2234588ead7` |
| R14 firmware branch | `fx/clp-r13-max-feedback-r14` |
| **R14 source commit** | **`00ea59d38de6f006dfdbcd472c2cfe5040a440c3`** |
| [R13 → R14 diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/149320f082f79edcb6a029f039fdb2234588ead7...00ea59d38de6f006dfdbcd472c2cfe5040a440c3) | Five files: mixer.c, main.c, R14 test, R14 packaging and R14 workflow |
| [GitHub Actions](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41661018210) | Run `41661018210`, attempt 1, successful; job `126003919111` |
| ARM toolchain | Arm GNU Toolchain 14.3.Rel1, `arm-none-eabi-gcc` 14.3.1 20250623; Release `-O3` |
| Documentation branch | `reports/2026-11-03-r14-clp-max-build` (report only, distinct from firmware source) |

Current documentation and evidence policy were read from `docs/lab-project-state` (`PROJECT_STATUS.md`, `WORKFLOW.md`, `EVIDENCE_INDEX.md`, `RECORDKEEPING_STANDARD.md`); firmware instructions were read at the actual R13 source commit, not inferred from documentation ancestry. The private KB v1.4.0 is a 2026-11-01 historical snapshot. Existing R13 image, CI, source, R7/R5 evidence and prior audio reports were reused; historical builds/tests were not rerun merely to recreate evidence.

## Implemented source behavior

**Only the CLP feedback fader selector was changed.** The R13 exact-single-route selector was replaced by a maximum over the existing six smoothed squared voice-fader values (`FaderVolumeFilter.b`) whose routing is `MIXER_ROUTING_FX_BUS`. Applying the monotonic R13 map once to that maximum yields the intended shared control:

`G_FB = min(1, 2*sqrt(max(q_routed)))`, equivalent to `min(1, 2*max(u_routed))` in steady state for physical fader positions `u`.

At least one routed fader above 50% retains full CLP feedback. With all FX-routed faders below 50%, the maximum sets shared feedback. Exactly one routed voice retains the R13 control curve. **With no configured FX routes, gain remains unity as in R13**; R5's idle gate remains in place. No activity detector, per-voice CLP instance, per-voice wet output, or independent Voice MIX/VOL changes were added. Silent configured routes are still included in MAX, so a routed silent voice with a high fader can hold the shared feedback open.

The selector `mixer_clipMaxRoutedFaderGain` is `noinline` in normal flash (`.text`); it is evaluated only at the beginning and end of each 64-sample output block for CLP. The existing R13 interpolated feedback ramp (`clipFaderFeedbackFirst`, `clipFaderFeedbackStep`) is retained, as is the original per-sample voice-fader smoothing. There are **no new state bytes or audio buffers**. A crossing of dominant faders within a block is approximated using endpoint maxima and interpolation, not sample-exact MAX across the entire block.

Unchanged production files were byte-compared to the exact R13 base in CI and independently rechecked in the downloaded source archive: `Distortion.h`, `Distortion.c` (R5 input-loss feedback gate, diode boundary fix, R7 parallel bass, R13 ramp), `FxBus.c`, all six synthesis-path voice sources and `adcPots.c`. Other effects, routing, DSP output and serialized preset formats were not deliberately modified. Source modifications: `Core/Src/Soundengine/DSPAudio/mixer.c` and `Core/Src/main.c` (splash `LXR LAB R14`, second line blank). Build/test-only additions: `.github/workflows/clp-r14-build.yml`, `tests/fx_clip_r14/package.py` and `tests/fx_clip_r14/max_control.py`.

## Reproducible checks and outcomes

CI used the R13-derived Release recipe and actual production DSP host fixtures:

- Production DSP baseline, R5 detector/hold/fade/reopen, R7 independent bass/state, FX-bus dispatch and reset fixtures: **PASS**.
- Inherited `tests/bug001`, `tests/kit_load`, `tests/project_storage`, `tests/euclidean`: **PASS**. Retained output logs disclose their exact scenarios, counts and host fixture substitutions.
- New `python3 tests/fx_clip_r14/max_control.py` extracts the **actual compiled R14 mixer helper**, provides typed host globals, and tests all 64 routing masks across 256 deterministic/seeded fader vectors (16,384 total). Zero-route 256, exactly one route 1,536, multiple routes 14,592: **PASS**, compared bitwise to the mathematical R13 singleton mapping of the routed maximum. A further 4,096 ramp-endpoint comparisons: **PASS**. This is *not* a whole-mixer timing or real-audio render test.
- Exact R13 CLP, input, bass, fader and effect files compared byte-for-byte in CI: **PASS**.
- Exactly one ARM Release compile/link, memory-region fit: **PASS**.
- Image `LXRV2IMG` header, recorded size, checksum, reserved bytes, R14 identification, stack/reset vectors, ELF load segments, flash application boundaries and ZIP one-image membership: **PASS** in CI, then **independently rechecked** on downloaded image/ZIP/ELF. Four nonempty ELF load segments match image bytes.
- Git blob hashes of the two production modifications in downloaded `SOURCE.zip` independently matched the live R14 repository: mixer `d173ae0e4f271ef91ba82128e347f28e42de8b87`, main `d163002cd7da187a70e9c0898e8148579c810d03`.
- Protected refs were checked after publication: `main`, R7, R11 and R13 remained at their recorded hashes; the historical R4 source was not changed.

## Matched R13 → R14 static memory and image comparison

| Measure | R13 | R14 | Delta / R14 remaining |
|---|---:|---:|---:|
| ITCM program | 15,536 B | **15,208 B** | **−328 B; 1,176 B free of 16,384** |
| DTCM linker used | 107,424 B | 107,424 B | 0 B; 23,648 B gap to top of DTCM, **not measured free stack** |
| General SRAM linker used | 274,648 B | 274,648 B | 0 B; 118,568 B before unmeasured heap/runtime demand |
| Linker FLASH | 235,092 B | 235,356 B | +264 B; 780,452 B available in application linker region |
| Complete application flash payload | 287,624 B | **287,560 B** | **−64 B** |
| Delivery IMG (includes 16-byte header) | 287,640 B | **287,576 B** | **−64 B** |

The MAP/ELF confirms that `mixer_clipMaxRoutedFaderGain` occupies 268 B in flash (`0x08022a55`, Thumb symbol) rather than ITCM. The ITCM mixer object drops from 3,808 B to 3,468 B; the net ITCM drop is 328 B after differences in linker stubs/alignment. The fast mixer function drops from 3,688 to 3,346 B. The increased flash code (+264 B) is more than offset in the total image by smaller ITCM load bytes (−328 B), producing a net 64-byte shorter update payload.

**Important unmeasured boundaries:** Real STM32F765 CPU cycles, flash/ITCM fetch penalty, audio render deadlines, DMA freshness, stack high-water, interrupt latency, long-term and multi-route audio stability, and actual sound are **UNMEASURED**. The two normal-flash helper calls per block could change execution latency despite lower ITCM use. No on-device acceptance or click-free transition can be inferred from host/CI success. The original R13 feedback curve is matched mathematically for solo routes; no new instrument listening was performed.

## Delivery and hashes

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R14.img` | 287,576 | `7880088bd61f3b7ad845cfaf0421615799c129ab1bfdb003fd6ee5df5aa470f9` |
| `LXR_V2_R14.zip` | 205,804 | `9dc9c52905704c262696c124fed7937bfb63d3a3de30cc92538669aeee83df97` |
| `SOURCE.zip` in CI evidence | 1,712,307 | `436b11f1696d7a72bd19954ed6ba46fb03477d69fd439cbf5b24d7592ddbee90` |

[Downloaded image/ZIP artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41661018210/artifacts/13240879838). [Source/ELF/MAP/build and full test-log evidence artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41661018210/artifacts/13242346334). Both GitHub Actions artifacts are **expiring, not permanent archives** (workflow configured for 30-day retention); the permanent report records their hashes and provenance, but does not itself archive their binary bytes.

## Acceptance boundary and next decision

The owner can compare R14 against the preserved R13 image under controlled, low monitoring level, focusing on exactly one FX voice (unchanged curve), 2+ simultaneous FX-routed voices (shared MAX), a high-fader silent route, fade around 15–5% for all routed voices, transitions in the dominant route, and no-step idle handling with R5. **No hardware test has been executed or authorized as part of this build.** Do not declare R14 musically accepted, flash the device, or begin follow-up DSP alterations without a separate decision. There is no Codex involvement or agent quota usage in this implementation.
