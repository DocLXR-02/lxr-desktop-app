# R20 corrective test build

**Task** LXR-R20-REC-STEP-NOTICES-2026-11-13  
**Date** 2026-11-13 (Europe/Vienna)  
**Status** Host and ARM Release CI PASS. Owner device validation pending.  
**Authorization** The owner approved the three R20 corrections and the inherited STEP issue, on a separate branch with an ARM CI build.

## Source and scope

| Item | Identity |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Branch | [fix/r20-rec-step-notices-2026-11-13](https://github.com/DocLXR-02/lxr-desktop-app/tree/fix/r20-rec-step-notices-2026-11-13) |
| Exact base | `b56e70ced8337476f1e302b61edcc3d718eb4e2f` |
| Exact built commit | [f40eff9b8938f57cb7f1c719211a23b2da236dc1](https://github.com/DocLXR-02/lxr-desktop-app/commit/f40eff9b8938f57cb7f1c719211a23b2da236dc1) |
| Verified tree | `e91561b0bf30d57bfbc581e5bf1975e5af92fb3d` |
| Prior firmware evidence | [Original R20 build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/5838f8bd80b935e2fe34a356aeacd1256bcbaca7/docs/codex-berichte/2026-11-13_r20-rec-ui-global-conflicts-arm-build.md), [original R20 CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43338109758), and `LXR_R20_Verification_2026-11-13.zip` with original failing host fixtures |
| Continuation references | Active private KnowledgeBase v1.5.6, source AGENTS.md, and canonical project status, workflow, evidence index and recordkeeping at `f3242aee4cf7af7909cf6c37c0b8af2e291f8104` |

The verified remote diff is one commit above the approved base and contains 13 paths. Production changes are limited to the sequencer STEP assignment and reservation logic, the STEP parser response, and LCD publication/priority. The remaining paths are CI and tests. The accepted FIX8 algorithm, capture engine, DSP/CLP, storage/serialization, bootloader and startup label have no diff. `main`, R4, R11, R18, R19, FIX8 and original R20 branch heads were checked after publication and matched their recorded identities.

## Corrections

1. **Shared target ownership during capture.** The destination check includes the pending live target from the first accepted change until the loop commits. It snapshots this reservation before scanning stored lanes, so a transition from pending to committed is covered. Direct STEP and hold-STEP share `seq_assignAutomationStep`. If a MIDI interrupt starts a new pending take during validation, the generation changes and STEP validates again before writing. Interrupt masking covers the generation comparison and at most two destination/value byte stores. The pattern scan runs with its normal interrupt state, and the loop-end commit gains no additional pattern scan. Existing cross-voice duplicates remain editable in place. See `sequencer.c` 2400–2423 and 2610–2708.
2. **Save Kit and Save Pattern visibility.** All three save-confirmation entry points use `menu_showSaveSuccess`. The newer save replaces older REC and kit-load notices and gets its own 1,500 ms window. The save page takes priority over REC notices generated during that window. A newer REC notice can appear after save if its own timer is still active. The existing SD-error priority is preserved. See `menu.c` 1835–1843, 2021–2031, 2254, 2304 and `buttonHandler.c` 1177.
3. **Expired REC repaint.** Notice publication updates the fields together in a short interrupt-masked section. The notice revision also changes on expiry and replacement. The renderer queues a fresh repaint if the revision changed while it was rendering. TIM14 remains responsible for expiration. Text formatting and LCD transfers run outside the new masked section. See `menu.c` 1992–2110.
4. **Rejected manual STEP destination.** The rejected P1/P2 request now sends the real stored destination back over the existing front-panel response path. This refreshes the menu immediately for an existing target or OFF. The rejected destination writes no step data. A later deliberate value edit therefore applies to the target now shown on screen. See `CortexParser.c` 39–46, 517–535 and 957–971.

The known FIX8 behavior where the REC LED stays lit after an in-progress take is invalidated by a context change remains outside these four corrections. The blocked-at-arming LED behavior passed regression checks.

## Test and build evidence

[Actions run 43399545352](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43399545352), job `131798680607`, attempt 1, push event, **SUCCESS**. The evidence manifest names the exact built commit. Compiler Arm GNU Toolchain 14.3.Rel1 (arm-14.174), GCC 14.3.1. The CMake Release build log contains no compiler warning or error matches. The run used the existing pinned compiler archive SHA-256 and produced one ARM Release candidate.

- Existing R20 host conflict and notice invariants PASS.
- Source-extracted integration with the production capture implementation and LCD refresh state machine PASS. It covers both REC tracks, all 12 global IDs across boundary voices and lanes, direct STEP and hold-STEP, the pending-target interleaving and a forced MIDI interrupt during STEP validation, menu restoration to a stored target or OFF, both save confirmations with overlapping REC, the forced expiry/preemption sequence, and all 1,499/1,500 ms and 32-bit wrap cases. Hardware IO and the STEP return cache adapter are modeled.
- MIDI note capture, silent loop, incomplete capture and repeat capture controls PASS in the host fixture.
- Existing capture, adaptive exclusion, project storage/transaction and Delay, USB-MIDI, Compressor and CLP host suites PASS.
- FIX8 numeric parity PASS across 16,384 endpoint pairs with 106,792 intermediate changes.
- Local `git -c core.whitespace=cr-at-eol diff --check HEAD^ HEAD` passed. The original source could be cloned through Git, while the direct CLI push lacked credentials. The approved branch was created through the GitHub connection with a tree hash identical to the tested local commit. One transient blob response was retried before branch creation. The first local fixture runs had two stale cache assertions that were corrected to reflect actual encoder input and rejection behavior before CI. No ARM run needed a retry.

Reproduce from the exact GitHub checkout with:

```sh
python3 tests/r20_ui/run.py
python3 tests/r20_ui/integration/probe.py . build/r20-ui-host
cmake --preset release
cmake --build --preset release --parallel 2
```

The workflow contains the full inherited regression commands and compiler setup. The original CI evidence archive contains source identity, compiler information, configure/build/section logs, the host suite outputs, ELF, MAP, image and output manifest. GitHub artifact `14023859019` has scheduled expiry 2026-12-13. Its exact original bytes are retained separately with this handoff.

## Build outputs

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R20_FIX_TEST.img` | 293,496 | `029855401310a0a9394946c069a2c24f8fba97050712b72ff14f6c9f7df44685` |
| `LXR_V2_R20_FIX_TEST.zip` | 210,066 | `a7afbde154fa75ba5d969bf65908fcc3a8a993154946a8d86500cc15dbd66d53` |
| `LXR_V2_STM32F765.elf` | 398,780 | `0078eef6b3ca067be928dc424f5171e1de62b307b8260e1ceb470a87b26a352c` |
| `LXR_V2_STM32F765.map` | 549,284 | `129be9ea147412d02f89526f6df81297fc202c43c93fd0e2c070e395e378fd1b` |
| `LXR_R20_Fix_CI_Evidence_43399545352.zip` | 531,393 | `f55977546ce27cd39800b931f9a129cf0749bd351e5c538309ab1b706affc51e` |

Independent validation passed CI artifact CRC, manifest hashes, IMG byte equality, updater magic `LXRV2IMG`, encoded payload length 293,480 B, checksum byte 0xD3, `(sum(payload) + 0xD3) % 256 == 255`, and startup text `LXR LAB R20`. The distribution ZIP passed CRC, has exactly one IMG entry and contains the exact CI image bytes.

## Static resources and remaining device evidence

Matched Release MAPs use the same linker configuration. Span figures include alignment. SRAM figures include the 512 B linker-reserved heap.

| Static bytes | Original R20 | Corrected R20 | Delta | Nominal gap |
|---|---:|---:|---:|---:|
| Flash `.text` | 155,960 | 157,056 | +1,096 | |
| ITCM | 15,304 | 15,304 | 0 | 1,080 |
| DTCM occupied span | 107,424 | 107,424 | 0 | 23,648 before runtime stack |
| General SRAM occupied span | 274,876 | 274,884 | +8 | 118,332 |
| Flash load payload | 292,384 | 293,480 | +1,096 | |
| Updater IMG | 292,400 | 293,496 | +1,096 | |

CPU/WCET, IRQ/DMA deadlines, stack high-water, long-term reliability, physical LCD visibility and listening results remain **UNMEASURED**. No hardware test, flash, merge, public release or canonical KB update was performed. R19 remains the approved stable firmware. The separately approved device pass can use the checklist in `LXR_R20_Verification_2026-11-13.zip`, with these exact image identifiers and the corrected cases above. The firmware source is published on the corrective branch. This independent build report is delivered privately.
