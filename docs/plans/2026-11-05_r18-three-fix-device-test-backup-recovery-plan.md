# LXR Firmware Lab — R18 Three-Fix Device Test, Backup and Recovery Plan

**Plan revision:** 1.0 — 2026-11-05 (Europe/Vienna)  
**Status:** **PLAN ONLY — NOT AUTHORIZATION TO FLASH, INSTALL, OPERATE DEVICE TESTS, MERGE OR RELEASE**  
**Scope:** One prospective, owner-supervised test of the independently built and integrated Delay, USB-MIDI and mono Compressor corrections on an Erica Synths LXR-02, with an explicit exit/rollback strategy.  
**Source of authority:** Owner-approved plan preparation following the successful combined host/ARM validation. The next operational steps require distinct explicit owner approval.

## 1. Controlled identities and evidence

| Role | Exact identity | Meaning |
|---|---|---|
| Musically accepted reference | R18 source `41b364067bce0d8cde624c184933d9cb88e2725b` | **Protected**, owner-accepted CLP sound; not to be overwritten in GitHub |
| Original accepted R18 firmware | `LXR_V2_R18.img`, **287,944 bytes**, SHA-256 `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1` | Candidate recovery reference, previously operated successfully on the owner's instrument |
| Integrated source | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` | Five changed production files only; direct child of accepted R18 |
| Integrated validation IMG | `LXR_R18_INTEGRATED_ARM_VALIDATION_ONLY.img`, **288,184 bytes**, SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7` | **NOT FOR FLASHING**, not a release/delivery package |
| Integrated host/ARM CI | [Run 42069839191](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191), successful | Build and selected host tests only; **no device behavior measured** |
| Integrated build report | [Immutable report](https://github.com/DocLXR-02/lxr-desktop-app/blob/8ce74dfb692e08a605d6a4817134f03fb21180ff/docs/codex-berichte/2026-11-05_r18-three-fix-integration-arm-validation.md) | Exact test coverage, resource counts, limitations |
| Original R18 recovery image provenance | GitHub R18 CI run **41725066117**, artifact **13274151339**, `lxr-r18-image` | Re-downloaded and independently checked against accepted R18 image SHA and ZIP SHA |

**Important identification limitation:** The experimental build still displays **`LXR LAB R18`** at startup. The boot text cannot distinguish the candidate from the accepted original. The approved image's SHA-256 and a dated installation record must establish installed-image identity; it cannot be proven by reading the LCD alone. No new version string, bootloader change or firmware rebuild is implicitly authorized.

## 2. Decision gates — do not skip

- **Gate A — proposed no-flash preparation (device interaction separately approved):** Inspect existing evidence, back up all SD data, verify original R18 recovery bytes, prepare a consistent baseline test configuration, and determine whether a compatible USB-MIDI test host and audio monitoring are available. *No bootloader entry and no firmware installation.*
- **Gate B — installation authorization (NOT GRANTED):** Owner reviews recorded Gate A results, service/warranty status of the intermittently failing DATA encoder, recovery limitations, and the exact proposed installation image/package. Only a separate explicit authorization can permit placing an installable image on the SD root and invoking the bootloader. A `NOT_FOR_FLASHING` validation artifact must not be silently repurposed.
- **Gate C — controlled device test authorization (NOT GRANTED):** Owner approves the specific listening, routing and USB tests, connected devices and signal-level precautions. If approval is withheld or essential gear is missing, leave the respective tests **NOT RUN**, not PASS.
- **Gate D — restore/release decision (NOT GRANTED):** Owner explicitly authorizes any rollback attempt, acceptance of the new sound, software release, or GitHub merge. A passed test is not automatically an authorization for any of these.

### Hard stop / no-go conditions

Do not proceed toward a device update if **any** of the following applies: working SD backup absent or unverified; accepted R18 recovery IMG missing or its hash differs; attempted firmware image identity/hash differs from a separately approved manifest; multiple ambiguous `.img` candidates on the SD root; SD read/write integrity errors; intermittent power or questionable power connector; device needs pending manufacturer inspection that could be affected by modified firmware; encoder cannot be operated reliably without abnormal force; active high-level audio monitoring; incompatible or unverified USB host; critical project work unsaved; or an unreconciled change to the approved integration commit.

**The existing DATA encoder bug is a specific risk.** The manual's firmware-update entry uses a held encoder press. A tactile click does not prove registration. Do not use extra force or assume repeated bootloader attempts are safe while a service diagnosis is pending.

## 3. Backup and return-to-reference preparation (Gate A, no flashing)

1. **Preserve unsaved work while the original R18 is still installed.** On the instrument, save any changed KIT, PATTERN and SONG as appropriate, then **SAVE + PROJECT** so that data persist to the SD card. The manual explicitly distinguishes RAM-only saves from the project save that writes the card. Do not use an unspecified autosave assumption. Confirm saves on the display; if the encoder fails to register, stop and document rather than forcing it.
2. **Power off normally.** Remove the SD card only after the device is safely off. Copy the **entire** SD filesystem, not only project folders, onto an independently stored computer/drive location. Retain an untouched, dated original backup; ideally make a second verified copy on a different medium. Do not format the original card or rename existing project files as part of the backup.
3. **Verify the SD backup.** Record card capacity, directory tree/file count and file sizes; calculate file SHA-256 values if a compatible host tool is available. Verify that copied important project files match their SD originals. Record any unreadable item as a **BLOCKER**. A backup of SD data is **not** a firmware/bootloader backup.
4. **Preserve recovery firmware separately from the SD project backup.** Use the previously downloaded original R18 CI artifact, which contains `LXR_V2_R18.img` and `LXR_V2_R18.zip`. The outer archive's ZIP CRC was checked; the nested delivery ZIP has the expected accepted SHA-256 `2f3ab9501a858e030babb499a9116fcc21fa0149fb38aa41f46a81a078205588`. Both image copies have SHA-256 `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1`. Keep an offline, verified recovery copy. **This does not establish that a later rollback flash will succeed.**
5. **Prepare a separate test SD card if available.** First verify that the LXR can load the intended reference project from that card *without firmware updates*. Do not rely on a clone until its copied files have been checked; preserve the primary card unmodified.
6. **Write down the exact pre-test state:** installed reference R18; project/kit/pattern identifiers, all FX settings, voice routing, audio output cabling, master/fader positions, BPM, MIDI channels and USB host/controller model. Photograph parameter pages where practical. Maintain a paper or offline recovery checklist independent of the LXR UI.
7. **Establish available equipment:** safe low-gain audio monitor/mixer, appropriate mono/stereo cables, compatible USB-MIDI host capable of logging incoming/outgoing messages, and a stable 12 V / 1 A center-positive power supply matching the LXR-02 manual. Do not assume the user's existing PC, DJ controller or iPhone can act as a compliant USB-MIDI host without a tested connection path.

**Backup completion condition:** all critical project bytes and the precise accepted R18 firmware image are available outside the test SD, and can be read and hash-verified before any installation authorization. A copy or image hash alone does not prove a successful future restoration procedure.

## 4. Pre-install baseline: accepted R18, same fixtures

Record *before* any candidate installation. Use disposable copies of Kits/Patterns, not the only saved original. This is a **proposed device test, not performed by this plan**.

| Test ID | Controlled fixture | Baseline evidence to record |
|---|---|---|
| B-01 | Boot and load known project, KIT and PATTERN | Boot indication, correct project content, load/save behavior, unexpected freezes |
| B-02 | CLP `typ=Clp`, input present at a fixed sequencer pattern and BPM (160 BPM may be used if it matches the owner's established reference) | Accepted bass, fluid feedback, soft-ceiling behavior, stable level across documented fader/dry-wet positions |
| B-03 | CLP idle / missing-step segments and voice-fader zero | Existing R18 no-unwanted-idle-whistle behavior; safe monitoring level |
| B-04 | Delay `typ=Del` and `typ=Pp` in distinct copied test Kits, with low `fbk` and documented `out`/`pan` | Left/right echo behavior and whether mono-right routing can be activated in the physical output configuration |
| B-05 | Compressor `typ=Compressor`, low/moderate signal and makeup `gai`, ratio `rat=1:1` | Subjective low-level reference, no clipping unexpected at normal settings |
| B-06 | USB-MIDI with compatible host at modest event rates, both RX and TX when the configured connection supports them | Host logs of note on/off, CC, known channels/route, no unexpected stuck notes in the modest-load fixture |

The manual describes **one FX processor at a time**. Therefore CLP, Delay and Compressor need distinct copied Kits or controlled changes, **not** an invented simultaneous multi-FX chain. Do not overwrite accepted source sound presets. Where physical output routing or MIDI direction cannot be established, mark the corresponding baseline test **BLOCKED** rather than assigning a result.

## 5. Prospective installation procedure — blocked until separate Gate B approval

This is a **reference checklist only**; do not perform these actions on the basis of plan approval:

1. Obtain a specifically **authorized installation candidate** derived from exactly integrated source SHA `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` with a separately verified delivery manifest and the intended updater filename. Keep the build report, actual IMG hash, file length and a distinct candidate identity in the installation record. The existing `VALIDATION_ONLY` image is not itself an authorized installation release.
2. Verify the SD copy's **actual bytes** and filename after transfer. Have **exactly one unambiguous LXR updater `.img` file** in the SD root. Preserve other SD content; keep any original/rollback image off that root until deliberately needed.
3. Confirm stable power and silent/attenuated audio. The owner's DATA encoder may intermittently miss presses; resolve its operational/service risk *before* attempting bootloader entry.
4. The supplied LXR-02 manual (p. 39, firmware update) describes placing `LXR_V2_xxx.img` in the SD root, holding the encoder while turning on, checking the bootloader display and firmware checksum, and **not turning off power while installation is underway**. It does **not** specify a guaranteed cancel/rollback procedure once installation starts.
5. In an approved session, document the displayed validation/update result and the final completed/rebooted state. Errors `Not Valid`, `Wrong size` or `Wrong Checksum` require **STOP, power-safe diagnosis and SD/image re-verification**, not random file renames, mass erasing, option-byte writes or blind repeated flashes. If updating starts, do not intentionally interrupt power.
6. After completion, record which **approved image hash** was installed. Because both R18 and this experimental candidate display `LXR LAB R18`, the startup string alone cannot establish the version. Device firmware readback is not available through the established test plan and is not presumed.

No bootloader, full-chip erase, boot-sector or option-byte modification is allowed by this plan.

## 6. Prospective post-install device test matrix — blocked until Gates B/C

Run in increasing risk order. Always begin with reduced downstream gain, a known test project and short patterns. Only change one diagnostic dimension at a time. Record actual values and audio connections; use identical fixtures to those used in the R18 baseline wherever comparison is meaningful.

| ID / priority | Setup and operation | Expected result / classification |
|---|---|---|
| T-01 / critical | Cold boot and project selection; test UI interaction, storage read and benign save to a disposable project | Boots repeatedly; reads the intended project; no unexpected reset, SD corruption or UI freeze. Differentiate known encoder symptoms from new regressions. |
| T-02 / critical | Repeat B-02/B-03 CLP patterns: kick, fader span, dry/wet controls, silent-step / fader-zero conditions | Owner-accepted R18 bass, musical feedback and ceiling behavior retained; no new idle whistle, severe level jump, dropout or objectionable coloration. A/B is a **subjective** device comparison, not mathematical acoustic equivalence. |
| T-03 / high | Delay `Del` in a disposable Kit; route FX output separately to left and right mono hardware outputs where the routing actually supports it; observe wet and dry/wet at low feedback | Echo timing/output channel responds; no hang, crackle, severe channel asymmetry, wild transient or audio loss. Confirm the actual tested mono-right routing; absent routing confirmation, do not claim to have physically covered the previously misaligned path. |
| T-04 / high | Delay `Pp` with **nonzero pan** (manual requirement); two output channels monitored at comparable gain; adjust `tim`, `rng`, `ton`, `fbk`, `d/w` conservatively | Alternating left/right echoes where configured, no new artifacts, lockups or missed returns. Avoid high feedback while initial stability remains unknown. |
| T-05 / high | USB-MIDI receive: slow controlled note-on/off and CC stream from a compatible logged USB host, followed by modest short bursts | At ordinary loads, notes/CC respond without systematic loss or persistent stuck notes; compare timestamps/counts where logged. Verify correct channel/routing first. |
| T-06 / high | USB-MIDI transmit: documented LXR sequencer-to-host MIDI output at modest event rates; log outgoing packets | Note-on/off pairs and supported clock/CC events match the selected routing as applicable; no unexplained duplicate/truncated events under ordinary loads. The manual has separate transmit/record/routing settings; record them. |
| T-07 / high | USB load and disconnect/reconnect only under silent/idle conditions, with every MIDI note released and transport stopped first | No crash, permanently blocked TX, corrupted state or unexplained persistent note after safe reconnection. **Exactly-once delivery across disconnect is not guaranteed**; log duplicates or omissions rather than treating all overload as PASS. |
| T-08 / high | Compressor normal range in a copied Kit, starting with low gain; compare `rat`, `gai`, `d/w`, and confirmed mono output against B-05 | No new difference in the comfortable normal range relative to original R18. This is a listening test, not a claim of exact unrecorded waveform identity. |
| T-09 / controlled risk | Compressor mono high `gai` with ratio `rat=1:1`, initially **very low voice/monitor gain**, stepping toward stronger input, comparing equivalent stereo path | No undefined numerical failure, crash, loud surprise or severe discontinuity. Saturation may be **audibly different by approved design** in the formerly undefined over-range domain. Safe listening cannot prove internal sample overflow actually occurred without direct sample capture. |
| T-10 / optional | Normal musical workload with multiple voices and FX mode switched only between separate Kits, moderate MIDI activity, approximately 20–30 min of continuous operation if resources allow | No observed crash, audible dropout, stuck note or output instability in *that* observed session. This is a bounded soak, **not** long-term reliability, CPU/WCET, IRQ/DMA or stack measurement. |
| T-11 / optional | Save, power down only after normal saves complete, cold boot and reload the disposable test project | Expected project content survives and no pre-existing data were changed. Compare against the SD baseline and backup; don't infer full power-loss safety. |

**USB limits intentionally remain:** The corrected TX queue can hold at most 31 complete four-byte events and RX up to 63; new events can be dropped at capacity, including Note-Off under sustained overload. Consequently, the test must distinguish modest-load correctness, expected bounded overflow loss, and unacceptable operational symptoms. Do **not** test high-rate MIDI with a loud or sustained sound. Use a safe all-notes-off/stop procedure; if notes remain stuck, stop the test and mute output before resetting.

**Compressor sound policy:** Owner-approved mono signed 16-bit saturation is an intended change only for the prior undefined overflow conversion, not a new accepted overall sound signature. Hearing no difference at moderate levels is not proof that over-range behavior was exercised. If the changed distortion is objectionable, mark T-09 **REJECTED** and retain original R18 as the accepted sound reference.

## 7. Observation and stop criteria

- **Immediate stop:** Unexpected piercing or rising feedback, monitor overload, stuck-note/sustained output that cannot be safely silenced, repeated resets/lockups, SD read/write failure, corrupted project, unexpectedly hot equipment/power fault, or observed bootloader/update error. Reduce downstream monitor gain or mute safely first; do not continue stress testing to collect more examples.
- **Audio result classification:** PASS (specified fixture satisfied), FAIL (specific new regression observed), INCONCLUSIVE (route/level not controlled or uncertain), BLOCKED (missing prerequisite/approval), NOT RUN (not attempted). Keep known R18 baseline anomalies separate from new candidate behavior.
- **Evidence:** For each test capture date/time, actual installed image hash *as recorded at installation*, source/branch, test ID, project/Kit/Pattern, FX values, `out` and `pan`, BPM, fader/master settings, cable map, MIDI host settings, stimulus/event count, audible observation, abnormal event and exact safe stop action. Optional phone video or audio recording must be labeled **un-calibrated**.
- **Performance limits:** Without debugger/profiler, **CPU/WCET, DMA/IRQ timing margins and stack high-water remain UNMEASURED** after any amount of subjective listening. Thermal/long-term operation is likewise not certified by a brief run.

### Per-test result template (copy once per executed test)

```text
Test ID:                 
Date/time and operator:  
Device firmware SHA-256 (installation record):
Project / Kit / Pattern: 
Audio outputs, monitoring gain, cables: 
FX type / OUT / D-W / parameters: 
Voice fader / BPM / MIDI routing: 
Host/controller / OS / software (if any):
Exact stimulus and duration / MIDI event counts: 
Observed output / recording or log filenames: 
Result: PASS | FAIL | INCONCLUSIVE | BLOCKED | NOT RUN
Difference versus recorded original R18 baseline:
Stop, recovery or follow-up needed:
```

## 8. Recovery decision tree — not a guaranteed bootloader rescue

1. **Minor unexpected sound/MIDI issue with responsive instrument:** Mute downstream audio, stop sequencer/host transport and release notes. Record firmware/fixture and symptom. **Do not write further project data** if SD state is uncertain. Retain current candidate installed until the owner separately decides whether to roll back; do not perform unapproved flashing.
2. **Project-only loss/corruption, firmware otherwise responsive:** Stop writing to SD. Use the independently verified complete SD backup to restore onto a separate or verified-good card, *after explicit approval*. Do not overwrite the sole original backup. Compare restored files/hashes and then test normal project load. Data restoration is not a firmware rollback.
3. **Firmware defect but bootloader still works:** Owner must separately authorize one normal **application-only SD update** to the byte-verified original `LXR_V2_R18.img` with SHA `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1`, on a clean unambiguous SD root with stable power. The manual indicates the normal loader procedure but does **not** promise downgrade compatibility or recovery after a failed flash. Verify a successful update and repeat basic R18 sound/project tests if rollback is attempted.
4. **Bootloader inaccessible, checksum/size failures that persist after verification, partial update, or repeated failure to boot:** **STOP.** Do not attempt blind erase, SWD/DFU recovery, bootloader update, option-byte editing or repeated firmware writes. Escalate to manufacturer/service with original SHA, image/hash evidence, failure display photos and SD manifest. The pending DATA encoder issue should also be communicated to the service provider.
5. **Only when baseline R18 boots, projects are intact and owner verifies the previous sound:** Record **RECOVERY CONFIRMED**. Otherwise record **RECOVERY UNVERIFIED/FAILED** with evidence and stop. A retained original image and successful historical R18 flash do **not** establish future recovery until actually observed.

## 9. Required completion record and next authorization

Gate A record must say explicitly: **SD_BACKUP_VERIFIED** yes/no; **R18_ROLLBACK_IMAGE_VERIFIED** yes/no; **TEST_HOST_READY** yes/no; **ENCODER_SERVICE_RISK_RESOLVED** yes/no; **SAFE_MONITORING_READY** yes/no; **CANDIDATE_INSTALL_IMAGE_APPROVED** no until separately authorized; **OWNER_FLASH_APPROVAL** no until separately authorized; **OWNER_DEVICE_TEST_APPROVAL** no until separately authorized. A single NO in an essential Gate A field stops installation consideration.

No candidate shall be called R19 or published as an accepted release merely because compilation and selected tests passed. After any separately authorized device session, add a distinct dated **English hardware-validation record** to the project evidence, with real device results clearly separated from host/CI and unmeasured timing limits. Any subsequent GitHub status/KnowledgeBase synchronization is a separate documented action; keep one authoritative current version.

## 10. Source references and boundaries

- Supplied **LXR Owners Manual** (`LXR_manual_2.pdf`): memory, project, SD and persistence pp. **8–9, 20–22**; FX routing, Delay, Compressor and `Pp`/pan pp. **25–27**; ordinary SD firmware update, checksum and error display p. **39**. Manual supports the described *procedure*, not a guaranteed recovery strategy or arbitrary file naming/selection across multiple root images.
- Owner-accepted R18 identity and observations: [GitHub current project status](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/PROJECT_STATUS.md), [owner hardware validation](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/HARDWARE_VALIDATION.md), and private KnowledgeBase v1.5.2. These documents precede the later integrated build; **the immutable combined report above is newer for its exact source/build evidence**.
- Combined correction and exact limitations: [2026-11-05 integration report](https://github.com/DocLXR-02/lxr-desktop-app/blob/8ce74dfb692e08a605d6a4817134f03fb21180ff/docs/codex-berichte/2026-11-05_r18-three-fix-integration-arm-validation.md) and [CI 42069839191](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191).
- Retention and evidentiary classification: [project recordkeeping standard](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/RECORDKEEPING_STANDARD.md). Uploaded evidence archives have finite GitHub Actions retention; local/private copies are not public firmware releases.

**End of plan. No firmware/device action has been executed by creating this document.**