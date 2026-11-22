# R23 — approved six-route snapshot CI validation (2026-11-22)

**Status:** CI PASS for the exact experimental R23 snapshot candidate; **not a stable firmware, device approval, release or distribution**. Report stored on a separate documentation-only branch. No firmware code was changed by this validation work.

## Identity, authority and scope

- Source repository: \`DocLXR-02/lxr-desktop-app\`; firmware branch: \`release/r23-block-routing-snapshot\`.
- Editor baseline: \`f8fc6fba3b7599095bd073460760b47ad49b173c\`; earlier authorized two-line input-row correction: \`36fa9a5a4b8dce156f42667342f4144ecc063c5e\`; authorized six-route block snapshot: \`164ab948035ca343959cbf57403e002d0db57231\`; committed snapshot host/ARM contract test: \`629975bb10ddd6c643b8fccf88194a013155c278\`.
- Exact CI workflow/head commit: [\`acda8a650fefc585b4ce61aeebc5238b8a6134c2\`](https://github.com/DocLXR-02/lxr-desktop-app/commit/acda8a650fefc585b4ce61aeebc5238b8a6134c2). Created \`.github/workflows/r23-routing-snapshot-validation.yml\` with a push filter for that one new file on the snapshot branch, so workflow creation triggered the explicitly owner-approved run. Allowed post-editor changes are exactly \`mixer.c\`, the snapshot test and the dedicated CI workflow; the CI checks lineage and path boundaries.
- Owner expressly approved full Host and ARM testing and then a dedicated GitHub Actions run on 2026-11-22. The separate TODO concerning CLP volume-only versus feedback behavior is **not** authorized for a sound change and was not implemented. No deploy, flash, CI release, PR, main merge or device test.
- Authoritative Lab main instructions were read at \`df47fe177833a9a206999d88e6910b13eb12b011\`; the current R23 case remains owned by \`DocLXR-02/lxr-firmware-lab\` on an unmerged staging branch.

## Completed CI: independently verified primary evidence

**Run:** [GitHub Actions #45043406222](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/45043406222), GitHub API status \`completed\`, conclusion \`success\`, exact run \`head_sha=acda8a650fefc585b4ce61aeebc5238b8a6134c2\`, event \`push\`.

**Jobs:** Host \`137272614211\` **completed success**; ARM \`137272614414\` **completed success**. Both run/job status and job logs were read independently; no failed steps.

### Host validation

The dedicated production-source-extracted snapshot test \`tests/r23_block_routing_snapshot/run.py\` **executed in the full GitHub checkout** (prior local isolated probe is not a substitute). Actual GCC ASan/UBSan host result:
- \`PASS: 100000 source-extracted snapshot/mask cases, CLP oracle, negative control\`.
- The deliberately flawed post-capture **live reread negative control detected 93,463 routing divergences**.
- Production \`SharedFxPool.h\`, extracted current source CLP fader selector and snapshot/mask statements were used in the generated host contract. Its companion \`clang -target arm-none-eabi -mcpu=cortex-m7\` object check also succeeded; **not** the full-firmware ARM build.
- Generated host probe SHA-256: \`9967135e9ca32f56d70ad2476e2062624012c8b5d56ee406b1a6b19881dd0bbf\`; contract ARM object SHA-256: \`4f1262fe95285e7a10c223bf25ef5506acb6bef796220cf1521a26c1ddd8cca9\`.

Additional inherited tests were run successfully: source-derived MIDI hardening, FX Kit format and migration, independent banks, FX voice UI routes, four real-FX/Kit integration (**5,832 64-frame six-voice scenes, 1,296 all-four-effect scenes**), editor/CLP state continuity (**44 paired real-DSP 64-frame snapshots per sanitizer**), shared pool routing, inherited R18 Delay/Compressor/CLP/USB, R21 voice copy, R22 drum copy, and project/legacy storage/transaction regressions. Passing host checks are bounded to their specific fixtures: they do not prove all real-time live events or unchanged physical sound.

**Host archive:** GitHub Actions artifact \`r23-snapshot-host-evidence\`, ID \`14762701876\`, 1,316,628 bytes (reported GitHub artifact size), expires **2026-12-22**. Original logs and generated host test evidence reside there; no permanent private archive import verified.

### Full ARM Release and static resources

ARM GNU toolchain **14.3.rel1** installed with retained SHA-256 check; CMake \`--preset release\`, \`cmake --build --preset release --parallel 2\` and final ELF link **PASS**. \`arm-none-eabi-size -A\`, exact IMG/ELF/MAP SHA-256 and size evidence recorded. Firmware image was generated on the runner solely as a build output and **not uploaded/distributed**. No hardware interaction.

| Section or aggregate | Earlier R23 editor \`f8fc6f\` | Snapshot CI \`acda8a6\` | Delta |
|---|---:|---:|---:|
| \`.text\` | 171,888 B | 171,880 B | −8 B |
| \`.rodata\` | 83,304 B | 83,304 B | 0 B |
| \`.itcm_text\` | 13,600 B | 13,608 B | +8 B |
| DTCM static \`.dtcm\`+\`.dtcmz\` | 109,324 B | 109,324 B | 0 B |
| General SRAM \`.data\`+\`.bss\`+\`._user_heap\` | 276,952 B | 276,952 B | 0 B |

Both builds used the R23 release workflow toolchain series 14.3.rel1. **Deltas are combined effects of the two-line FX input-row correction AND the six-route snapshot**, not an isolated snapshot-only comparison. Static free nominal ITCM = **2,776 B**; DTCM = **21,748 B before stack**, general SRAM = **116,264 B**, all bounds checked PASS. The six copied route bytes have **unmeasured** actual compiled stack-frame/padding overhead, and no target CPU/WCET, DMA deadline or stack high-water measurements exist.

- IMG: \`build/release/LXR_V2_V1.7.img\`, **307,184 bytes**. SHA-256 \`133a62bb0ab26ab7376cc6c7b4f6b93f5878008350f83c56724fbb77ce3249c6\`.
- ELF: \`build/release/LXR_V2_STM32F765.elf\`, **415,668 bytes**. SHA-256 \`f95fe21454cc8a5c5407a28e5f71182e1922679bb2fb4d348dbf72e2d37581d2\`.
- MAP: \`build/release/LXR_V2_STM32F765.map\`, **555,764 bytes**. SHA-256 \`8779f849fd51ad3069f66115eef34d6770937d04eee837faf4305457d33d9f90\`.
- The CMake build emitted compiler warnings in \`mixer.c\` about comparisons between two enum types and misleading indentation. These were **nonfatal** and were not modified within the approved snapshot scope. They are not evidence of passing runtime timing.
- **ARM archive:** GitHub Actions artifact \`r23-snapshot-arm-evidence\`, ID \`14762139147\`, **54,333 bytes** reported, expires **2026-12-22**. Contains source identity, toolchain, configuration/build logs, static section output, MAP and hashes but no IMG.

### Explicit exclusions, risk and next decision

Source/CI correctness is favorable within the tested scope. Snapshot uses one set of voice routes per 64-frame block *after* LFO/velocity modulation; active mask, all voice dispatch and both CLP gain selectors share it. **Still not established:** full simultaneous Kit-parameter atomicity, device live FX switches and audio perception, target CPU/WCET, IRQ/DMA deadline margins, runtime stack high-water, long-duration stress, or physical hardware behavior. CLP volume-only/feedback TODO is a **different prospective sound change**, to be discussed and approved separately. Maintain experimental status; R22 TEST03 remains stable, R18 CLP remains accepted sound reference.

**STOP:** No additional CI runs, software edits, releases, merges, flashing or device actions automatically implied by this completed test. Preserve and evaluate the two artifacts before expiration if durable original evidence is required.

**Report storage:** This Markdown-only handoff is committed on \`docs/r23-snapshot-ci-report-2026-11-22\` based on \`docs/lab-project-state\` at \`672891c23809e8a2014457f564c16732f970e4f9\`; firmware branch and both repository main branches remain distinct.
