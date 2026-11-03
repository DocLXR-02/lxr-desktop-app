# R15 experimental CLP HP1=110 Hz, no parallel bass — build and validation report

Date: 2026-11-03 (Europe/Vienna). Method: ordinary Chat/GitHub connector + GitHub Actions, **no Codex, Work, merge, release or device flash**. One explicitly authorized audition candidate, not sound acceptance. All new project documentation is in English.

## Scope and immutable identities

| Item | Identity |
| --- | --- |
| Repository | `DocLXR-02/lxr-desktop-app` |
| Musically validated R14 base | `00ea59d38de6f006dfdbcd472c2cfe5040a440c3` |
| Firmware branch | `fx/clp-hp110-no-parallel-bass-r15` |
| **R15 tested source commit** | **`03bcb4f4e21d368d7a5eda2691ad619bb595d994`** |
| Diff | [R14 to R15](https://github.com/DocLXR-02/lxr-desktop-app/compare/00ea59d38de6f006dfdbcd472c2cfe5040a440c3...03bcb4f4e21d368d7a5eda2691ad619bb595d994) |
| CI | [Run 41695560651](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41695560651), attempt 1, **success**, one Release ARM build |
| Build toolchain | Arm GNU 14.3.Rel1, `arm-none-eabi-gcc` 14.3.1 20250623, Release `-O3` |
| Report-only branch | `reports/2026-11-03-r15-hp110-no-bass-build` |

Owner's R14 device listening (Nov 3): extensive voice and FX-route combinations including all six voices and many CLP control settings were musically successful; R14 MAX feedback did not exhibit objectionable inter-voice interruptions. Open areas: solitary fade feedback loudness (approved for a **later, separate** experiment), kick bass loss with partial dry/wet, and occasional dense six-voice overload (explicitly deferred). This R15 task changes **only** the bass architecture for a listening experiment: use the existing core HP1 at 110 Hz and **eliminate** the separate R7 bass lowpass, accepting likely significant changes in feedback timbre. The owner favors possible longer tails, but the actual duration and real-time safety are unknown; no further release-envelope work was authorized.

The current private KB v1.4.0 is a historical 2026-11-01 snapshot. Checked current docs `docs/PROJECT_STATUS.md`, `docs/WORKFLOW.md`, `docs/EVIDENCE_INDEX.md`, `docs/RECORDKEEPING_STANDARD.md` from `docs/lab-project-state`, plus source-local `AGENTS.md`, the actual R14 firmware files, the R14 build/report and the preceding Nov 3 offline HP1 survey. Reused previous synthetic HP1 evidence rather than rerunning the full sweep. No Codex quota was used; current available Codex quota remains unknown. No new unrelated development task started.

## Production changes (only three files)

1. `Core/Src/Soundengine/DSPAudio/FX/Distortion.c`: Replace the three **active** HP1 coefficients in the stereo L/R and mono paths with `CLIP_HP1_ALPHA = 0.015536190543605555`, calculated as `1-exp(-2*pi*110/44140)`. This is a matched-pole **110-Hz** coefficient; it does not reproduce the original comment's 7.2-Hz request or preserve the shipped 7200-Hz core voicing. HP1 remains **inside the shared input-and-feedback path**, so both excitation and CLP self-oscillation frequency characteristics are intentionally changed.
2. Remove `Distortion_clipBass`, `CLIP_BASS_LP_ALPHA`, `CLIP_BASS_WEIGHT`, their samplewise processing and their wet sum in mono/stereo. The wet output now derives from `lastOut` alone. Retain the original R7 **core input trim `0.22`** to isolate the experiment.
3. `Core/Inc/Soundengine/DSPAudio/FX/Distortion.h`: Remove the two no-longer-used `double clipBassLp_state[2]` fields (16 static DTCM bytes). `Core/Src/main.c`: boot identification `LXR LAB R15`, second line unchanged (blank).

Unaffected: R5 linked input-idle gate, deferred audio-owned reset, diode-table boundary fix, original downstream ~60-Hz HP3, filter/tone/drive/VOL controls, dry/wet formula, R13 block interpolation and R14 shared MAX fader function, existing voice fader smoothing, FX routing, other effects, preset/kit format, output gain. R14 mixer, FxBus and all native voice/fader files were byte-compared in CI to the exact pinned base SHA.

No separate CLP cores, extra filters, buffers, dynamic normalization or master limiter were introduced. The per-voice independence of FX control was not altered. The open problem of solitary feedback pitch remaining loud until collapse was explicitly deferred to a separate authorized design.

## Tests and limitations

- Native host **unit test:** R5 input detector, 441-frame hold, 221-frame fade, reopen, reset, diode endpoints, Tube/Fold returns, mono/stereo checks: **PASS**.
- **Important prior test mismatch:** Original inherited R5 unit's assertion of fully quantized-silent output after ~22 ms of post-gate filtering **failed in local preflight**, as expected when HP1 drops from 7200 to 110 Hz. The R15-specific copy explicitly allows ~250 ms for filter-state settling before asserting full output silence. This is a deliberate, separately documented timing expectation change, **not** an unchanged R5-output-timing result. Gate counter/fade logic itself was preserved. Existing R5 historical tests were not modified.
- R15 core-state test: reset from stale state, input preservation, mono/stereo, linked detector, 2-second DC/int16-edge stress, 10,000-sample idle settle, bounded core-only wet output: **PASS**; observed maximum `abs(wet)` ~`0.647478998` under its fixtures.
- Actual FX-bus dispatch/reset over six routes: **PASS**.
- The reusable original R14 MAX helper test: **16,384 routing/fader cases and 4,096 interpolation endpoints PASS**; the mixer source was byte-identical.
- **135 production DSP whole-waveform + internal core-return-trace comparisons PASS**, testing R15 against a temporary **exact R14 DSP source** with only HP1 = 110 Hz and R7 bass output weight = 0, while retaining the old bass calculation in the reference. This validates the exact functional consequence of *removing* the bass process in this representative synthetic matrix: mono/identical/asymmetric stereo, five input histories, three control rows, three dry/wet levels. It does **not** prove sonic likeness to the R14 original.
- Existing `tests/bug001`, `tests/kit_load`, `tests/project_storage`, `tests/euclidean`: **PASS** using the repo's host fixtures/mocks.
- One ARM Release build, link/region fit, image header/embedded checksum, vector/flash-load-segment checks, one-image delivery ZIP: **PASS**.
- Independently re-downloaded and checked all exact image/ZIP bytes, GitHub source archive, production-file SHA-256 vs locally preflighted files, source commit, ELF flash-backed load segments, start vectors and link resources: **PASS**.
- The earlier offline HP1=110 survey (owner-provided November 3 archive) showed changes in 30–150 Hz RMS that are highly input/control dependent; do not interpret a near-neutral 50% D/W case as a universal response. Those data are **prior local HOST evidence**, not CI hardware measurements.

**NOT MEASURED:** actual STM32 CPU cycles and peak ISR/DMA deadlines, stack high water, hardware loudness, true 1–2-second decay, runaway/unstable resonance during user performance, audible clipping/intermodulation, stage/PA safety, actual physical kick bass and voice-1 timbre. A numerical core bound in the static host test cannot certify output/master-bus or hearing safety, and changing a coefficient in the common feedback filter can alter oscillation thresholds. No hardware installation or instrument test was carried out by the assistant. Owner should preserve the verified R14 rollback image and use minimal initial monitoring gain.

## R14 → R15 static resource ledger

| Measure | R14 | R15 | Delta / R15 reserve |
| --- | ---: | ---: | ---: |
| ITCM program | 15,208 B | **15,208 B** | 0 B; **1,176 B free** |
| DTCM static linker used | 107,424 B | **107,408 B** | **−16 B**; 23,664 B to nominal top of stack, actual stack use unmeasured |
| General SRAM static | 274,648 B | 274,648 B | 0 B |
| Linker FLASH | 235,356 B | **235,140 B** | **−216 B** |
| Complete application flash payload | 287,560 B | **287,344 B** | **−216 B** |
| Firmware IMG incl. 16-byte header | 287,576 B | **287,360 B** | **−216 B** |

In R15, `.text` shrank 151,416 → 151,200 B; `.dtcmz` shrank 70,424 → 70,408 B; `.itcm_text` remained 15,208 B. Removing a per-sample 150-Hz lowpass computation plausibly reduces CPU arithmetic, but **CPU saving is unmeasured**, and the changed HP1 coefficient is not an independently measured CPU optimization. Code-size movement in the linker is not a WCET/real-time guarantee.

## Delivered immutable evidence

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `LXR_V2_R15.img` | **287,360** | `9d48e2e18bf359e8f9057681c3d56cf6e43570c2d14d716100fb3f93b1dd7dfd` |
| `LXR_V2_R15.zip` | **205,637** | `60595a644454439b10171c633fd1cb9a3739b2f139e19746b5021bfd457aba0e` |
| Evidence `SOURCE.zip` | **1,719,084** | `cf214080066b2a07838a5eca6cec8cb9215acd9db034650c8894c32d1461a920` |
| R15 build ELF | **391,680** | `7eb35f98022af70a57ca45c2f3b23bd82343a9e505ceaefe1ce67f9787c98b21` |

[Image and delivery ZIP artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41695560651/artifacts/13262125337). [Source, ELF, MAP, full regression logs and hash evidence](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41695560651/artifacts/13262460247). These are **expiring 30-day GitHub Actions artifacts**, not permanent binary archives. This report and branch retain the reproduction recipe, scripts and cryptographic identities; their binary payloads remain available in the locally delivered files and current CI artifacts but are **NOT PERMANENTLY ARCHIVED** to a separately approved durable binary store.

## Workflow/reproduction and protected baselines

R15 source-only branch additionally contains `.github/workflows/clp-r15-build.yml`, `tests/fx_clip_r15/compare.py`, `unit.c`, `state.c` and `package.py`. They document the exact host/compiler invocation, keep the inherited original test intact and create the single-image delivery ZIP. CI uses the same Arm GNU 14.3.Rel1 release toolchain and source packaging as R14. The test-specific 250-ms upper bound is a **host-fixture requirement**, not measured physical tail duration.

After build and report preparation, all protected GitHub refs were independently rechecked unchanged: `main` `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`, R4 `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`, R7 `6d7cb7f7763158ed9abc465a72df7f2560f26b57`, R11 `64263ed25f2e9c7f5d880b493d943bfba29271a8`, R13 `149320f082f79edcb6a029f039fdb2234588ead7`, R14 `00ea59d38de6f006dfdbcd472c2cfe5040a440c3`. No merge, force-push, flash, external release, or subsequent firmware variant was performed.

**Acceptance next step:** owner-controlled low-level comparison against original R14, testing single voice-1 kick at dry/wet 0/25/50/75/100, one/three/six FX routes, sweep COL/DRV/TON/VOL, short/no-step patterns, and dominant-fader transitions. Listen for excessive low-frequency rumble, sustained oscillation, near-silence transients and heavy clipping. Do not approve R15 as new musical baseline until the owner reports the audition outcome. All other issues remain outside R15 scope.
