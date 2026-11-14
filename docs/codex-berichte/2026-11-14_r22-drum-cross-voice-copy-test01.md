# R22 — Drum 1–3 Cross-Voice Copy Between Kits — TEST01

Date: 2026-11-14 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**SOURCE, HOST TEST and CI/BUILD PASS. Test candidate NOT FOR FLASHING without
separate owner device-test authority. No hardware, flash, merge or public release ran.**

## Authorized scope and source identity

The owner selected **Alternative C** after the retained R22 same-kit stop report:
all nine Drum1–3 source/destination combinations between different kits are supported;
every same-kit transfer rejects, including different Drums. Same-kit synchronization,
isolated live LFO application, audio-boundary commands and modulation-engine changes
are excluded. This owner decision supersedes the original R22 same-kit requirement.
The stable R22 topic is now cross-kit Drum copy; the release number is unchanged.

- Exact R21 base: `861c643632e0a88bdc7d60d9410befb3971489fc`.
- Final source: [`ace7ad95081aac6a015f11b87b6c540ce100b344`](https://github.com/DocLXR-02/lxr-desktop-app/commit/ace7ad95081aac6a015f11b87b6c540ce100b344),
  branch [`release/r22`](https://github.com/DocLXR-02/lxr-desktop-app/tree/release/r22),
  one commit with parent exactly the approved base. The branch was absent at resume.
- [Exact source comparison](https://github.com/DocLXR-02/lxr-desktop-app/compare/861c643632e0a88bdc7d60d9410befb3971489fc...ace7ad95081aac6a015f11b87b6c540ce100b344).
  Twelve changed paths; patch/path manifest is retained with the evidence.
- Report-only branch: `reports/r22-test01-2026-11-14`, based on unchanged
  `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`. It contains only
  this report and `docs/evidence/r22-drum-cross-voice-copy-test01/`, not firmware.
- Read/reused source AGENTS.md, private KnowledgeBase v1.5.8 §§0.0/6 and current
  project status/workflow/evidence/recordkeeping at docs SHA
  `9b68be76d80de50cb84fa9882c5b9003bcb6faf8`. Private KB content is not published.
- Reused [R21 TEST01 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/30ab66c09217330d16e595f149bcab239b9bfad2/docs/codex-berichte/2026-11-14_r21-voice-copy-paste-test01.md),
  CI43621319613 attempt1/job132527438779/artifact14123628907, exact base, GNU14.3.Rel1
  Release. Original archive CRC/source/SHA-256 verified again:
  `ff0d61158dc2e89e25f85db92342dc09398b919135456a4b8197d8671ab65963`.
- Reused the immutable [R22 stop report and reproductions](https://github.com/DocLXR-02/lxr-desktop-app/blob/6ba37b023857a02b63ace0f1faa2fde24b4dd33a/docs/codex-berichte/2026-11-14_r22-drum-cross-voice-copy-stop-gate.md).
  Its live LFO reset and stored-only same-kit persistence failures remain historical
  evidence. The owner removed that execution path from R22; no workaround was added.
- OWNER HARDWARE: the task records R21 TEST01 successfully tested by the owner on
  2026-11-14. Matrix/duration/photo/timing were not supplied; this is functional
  acceptance, not a new stable-release declaration or R22 hardware authorization.

No subagents were used. Product model/effort selection and live quota/reset cannot
be independently verified here; UNKNOWN. R18 CLP source/sound remains protected.

## Final controls, ownership and persistence

With transport stopped in Voice Edit, select Drum1/2/3, hold unshifted COPY and
press DATA. Release COPY and DATA. DATA rotation selects the destination kit;
physical Drum1/2/3 buttons select the destination Drum. Default destination Drum
matches the source. Example: `D3 > K02 D1?` / `DATA yes COPY no` on the two16-character
rows. Destination presses, held edges and their releases are consumed without
changing source voice/track, starting track copy or becoming confirmation.

A fresh DATA press confirms; COPY cancels. Other controls follow R21 cancellation,
with START/STOP operating normally and running-transport confirmation rejecting.
Entry/release/bounce guards are retained. Every same-kit target rejects with
`rejected`; no cross-type target is permitted. Snare/Cymbal/shared HH retain R21
same-instrument transfer between different kits, including both HH decay values.
The R21 normal DATA/COPY/CLEAR/REC/STEP operations remain.

`success!` follows the committed inactive target RAM record. Source edits, loaded
kit, active sound, pattern association and Load/Save cursor remain intact. The
transfer loads no target, dispatches no engine/FX/MIDI/automation event and performs
no SD write. SAVE+PROJECT persists the target and source edits using the inherited
approved loaded-kit identity correction; separate SAVE+KIT is not needed.

The approved Morph guard is unchanged: for the currently referenced Morph target,
nonzero amount OR any pending sweep rejects with `Morph active`, including zero
amount/pending. Zero/idle permits transfer. Unrelated target kits remain permitted
during Morph activity. The guard/context are checked again at final RAM commit.
Same-kit targets reject before any write regardless of Morph state.

## Implementation and scope

Production changes are limited to `Core/Src/Preset/voiceTransfer.inc`,
`Core/Src/menu/voiceTransfer.inc`, one API declaration and the R22 boot string.
No supporting live-engine dispatch file needed modification. The remaining paths
are the focused tests/runner, bounded CI workflow and boot-label assertion.

An explicit table names equivalent saved Drum IDs for25 base fields: oscillator,
FM, envelopes, transient, filter, drive, local sample-rate, volume and pan. The
existing R21 base whitelist determines copied fields. Additional semantic rows
translate eligible LFO destinations referring to local LFO/velocity controls;
they add no base-field copies. No numeric parameter-range offset or display-string
mapping is used. The independent production menu schema validates the correspondence.

The new `preset_transferVoiceTo(copy, kit, voice)` retains the R21 staged RAM
transaction. The old API defaults to the source voice for existing callers. All
same-kit transfers reject before staging. Cross-type/non-Drum retargeting rejects.
Output routing, FX, velocity base settings, Morph/global/transport/placeholder
fields, other voices and every pattern/project field are preserved.

For an eligible nonzero source own LFO with a valid actual own-voice destination,
the destination Drum's corresponding LFO is overwritten with all eight saved
fields. The decoded destination is translated semantically, then re-encoded from
the actual `modTargets` table with the correct target-voice field. Retrigger selection
is copied unchanged, including a chosen other track. OFF/zero/invalid/other-voice/
FX/global/voice-field mismatch retains the destination LFO. Every other target-kit
LFO assigned to that exact mapped destination becomes OFF, including zero-amount
competitors; other fields/destinations remain intact. Normal later kit loading uses
the unchanged engine path and updates the assignments/base values.

The preflight live-engine problem is avoided by the approved different-kit-only
scope: confirmation never calls that operation. Engine/parser/modulation/mixer/FX
code, encoder implementation, file formats, R21 identity correction and sound
algorithms are byte-identical to the base. An input-hash manifest confirms relevant
unchanged files. No same-kit buffer, queue, live transaction or persistent clipboard
was introduced. The session snapshot is disposed of on exit as in R21.

## Executed verification

[Primary commands/fixtures/adapters](../evidence/r22-drum-cross-voice-copy-test01/README.md),
[run ledger](../evidence/r22-drum-cross-voice-copy-test01/run-ledger.json),
[final R22 CI log](../evidence/r22-drum-cross-voice-copy-test01/CI_r22-host.log).
Local GCC14.2.0/Python3.12.14, ASan/UBSan, deterministic inputs; no random seed.
The dedicated workflow pins all compile, regression and CMake commands.

| Executed check | Result |
|---|---|
| R22 `tests/r22_drum_copy/run.py` | PASS locally and CI: all9 cross-kit pairs with25-field independent menu oracle, exact all-kit/active/pattern/song/name/global preservation, zero transfer dispatch/IO, all9 same-kit rejects, invalid/cross-type/null rejects. |
| R22 LFO tests | PASS all9 slot combinations with all8 fields, high-ID destination remap, zero/nonzero exact competitors, retained other destination, project roundtrip and load dispatch. All306 eligible destination translations match independent menu semantics and actual tables. Seven ineligibility categories retain the destination LFO. |
| R22 persistence/panel/context/Morph | PASS all9 persistence pairs: held-STEP base snapshot, unsaved source, temporary recovery, cursor aliasing target, real-FatFs project save/reload, normal/immediate loads. Production snapshot helper runs; the fixture models unchanged STEP release restoration before save. Target-button/held/release, source selection, cancel, non-Drum behavior, pending-pattern/clock/commit and remapped Morph guards pass. |
| Supplemental former LFO destination check | PASS18 local cases on the exact final source, after CI without source changes: prior non-OFF own destination overwritten, former destination loaded base or retained remaining owner, real project/load path, zero confirmation dispatch/SD. |
| `tests/r21_voice_copy/run.py` | PASS local/CI; all inherited R21 behavior and retained57→123 Morph guard cases. |
| R20 UI and integration | PASS local/CI, including REC/STEP, shared conflicts, displayed targets, notices and MIDI interleaving. |
| Project storage/transaction suites | PASS local/CI, real FAT16/FAT32 media, normal save/reload and retained fault/power-cut/recovery groups. |
| Capture/FIX8 playback | PASS CI; capture scenarios and exhaustive16,384 pairs/106,792 intermediate changes. |
| BUG001 parser/sweeps | PASS local/CI; low CC/CC2 boundary,256 Morph positions, reserved transport exclusions and immediate load/restart behavior. |
| Delay / USB MIDI / Compressor | PASS local/CI, retaining expected original-source reproductions. |
| Accepted R18 CLP comparator | PASS local/CI135 return-state comparisons; no CLP retuning. |
| ARM Release / image validation | PASS matched GNU14.3.Rel1 Release, updater/ELF/IMG/boot/ZIP and static fit. |
| Independent retrieved-delivery check | PASS source, archive digest/CRC, four output hashes, updater/checksum, boot bytes, ELF segments and one-image ZIP equality. |

The R22 runner reuses R21's production-path fixture through a small optional-case
entry point. It does not clone all prior harnesses. HAL/LCD/physical media,
unrelated button helpers and parameter-application/modulation effects are explicit
host adapters. Actual selector/transfer/dispatch/load bodies, target tables and
FatFs RAM-media serializers run. These prove data/control ownership and assignment
paths, not complete audible rendering, physical contact timing or target deadlines.
Other retained target modulation/automation/routing/FX can shape the copied sound.

The resumed implementation and its host/CI builds passed without a corrective
source/build iteration. The earlier R22 stop reproductions and R21 pre-guard Morph
failure remain linked and were not relabelled new failures or silently overwritten.
No speculative full-engine audit or prior hardware test was rerun.

## CI/BUILD and files

[Run43639972875](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43639972875),
attempt1, [job132585174084](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43639972875/job/132585174084),
SUCCESS at exact source `ace7ad95081aac6a015f11b87b6c540ce100b344`.
Ubuntu24.04, Arm GNU14.3.Rel1/GCC14.3.1, CMake Release. All required host steps pass;
retained configure/build logs contain no compiler warning/error matches.

Artifact [14130863884](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43639972875/artifacts/14130863884),
`r22-voice-copy-build-evidence`, expires2026-12-14T20:55:26Z.
Original archive `LXR_R22_TEST01_CI_Evidence_43639972875.zip`,753,462 B, SHA-256
`3cdd0595699f83affe13619a14f74984e9d862a3c305c0298e0041808457e379`.

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R22_TEST01.img` | 296,720 | `54898850b369a824b917e6d2d13b9aeb82675e5fd2a1d6fcf612292193290dba` |
| `LXR_V2_R22_TEST01.zip` | 213,003 | `80825fe522ea861509074296b449ecf9e585691e8ea8c89f3e305fae5faaeda3` |
| `LXR_V2_STM32F765.elf` | 404,204 | `32e7cb2d2cd2b5942061077134cf6cc700d1c0ce72b45fafbab49892619bd177` |
| `LXR_V2_STM32F765.map` | 552,344 | `f9d6d1c827b87858850a5fdeeb9d7fb75a5ea66c59012c84bdab8de320f33112` |

The delivered ZIP is the CI-produced ZIP, with exactly the unchanged original-named
`LXR_V2_R22_TEST01.img`. Updater magic `LXRV2IMG`, payload296,704 B, checksum0xff,
reserved bytes zero. `LXR LAB R22\0` occurs at IMG byte160,680, with exact bytes
recorded in `delivery-verification.json`. ELF load segments match the image byte
for byte. This is binary boot-image evidence; no device-screen photo was taken.
The source startup retains a cleared second line. No additional candidate was built.

## Matched R21 → R22 resources and limits

| Static bytes | R21 | R22 TEST01 | Delta | Nominal remaining |
|---|---:|---:|---:|---:|
| Flash `.text` |159,440|159,856|+416|Part of complete load span|
| ITCM span |15,304|15,304|0|1,080|
| DTCM occupied span |107,424|107,424|0|23,648 before runtime stack|
| SRAM span including reserved heap |275,156|275,156|0|118,060|
| Complete Flash load payload |296,152|296,704|+552|719,104|
| Updater IMG |296,168|296,720|+552|Not a runtime allocation|

All linked regions fit. `.rodata` grows83,032→83,168 (+136). Initialized runtime
sections are included in the Flash load span, without double-counting. The SRAM
row is an aggregate occupied-span result, not a claim that no logical state was
added: the selector adds destination/held-button bytes within the resulting layout.
The semantic table is read-only. The foreground staging array remains247 bytes.

Verified ARM entry instructions give the transfer function's saved registers plus
fixed SP allocation as312→320 B (+8), recorded with prologue bytes and ELF hashes
in `stack-entry.json`. This excludes callers/callees and interrupt nesting, and
is not runtime high-water or a whole-call-tree bound. Extra mapping work remains
in the stopped-transport foreground path, with no new audio-ISR/render algorithm.

CPU/WCET, IRQ/DMA deadline margins, actual stack high-water, long-term reliability
and R22 physical sound tests remain UNMEASURED/NOT RUN. Static fit and native test
duration do not establish Cortex-M7 timing reserve or device acceptance.

## Retention and owner review

Small primary logs, commands, test inputs, source patch, manifests, validation JSON,
stack-entry decoder and SHA-256 list are retained with this report. Original
ELF/MAP/IMG/ZIP are in the expiring CI artifact and separately supplied archive.
An owner-managed permanent binary archive is NOT ARCHIVED / NOT VERIFIED; local
workspace/chat copies are not claimed permanent. R21 and original R22 stop reports
are preserved, along with all stable/protected references. Current public status/
evidence-index synchronization is a separate gap; no stable designation changed.

No correctness/build stop remains in the approved Alternative C scope. Every
same-kit transfer remains intentionally unsupported/rejected. The next owner/CHAT
step is independent review of this exact candidate. Separately authorized device
acceptance can check D3→K02D1, all9 different-kit pairs, same-kit rejection, unchanged
source sound/cursor, saved project reload, copied LFO/retrigger and Morph rejection,
then compare retained operations and R18 CLP listening conditions. No device work,
installation, flashing or follow-up task is authorized or launched by this report.
