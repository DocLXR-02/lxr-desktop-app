# R18 USB-MIDI — TX/RX Ring and CDC Ownership Host Report

**Date:** 2026-11-05 (Europe/Vienna).  
**Status:** Host validation **PASS**, no firmware build, device test, flash or release.  
**Authorization:** Owner explicitly requested continuing with item 2 (USB-MIDI), after the separately validated Delay candidate. Scope: USB buffer and transfer ownership only. No authorization for ARM build, R18 sound modification, merge, hardware installation, or Compressor work.

## Exact identities and source protection

| Item | Identity |
| --- | --- |
| Repository | DocLXR-02/lxr-desktop-app |
| Accepted original R18 source | **41b364067bce0d8cde624c184933d9cb88e2725b** |
| Independent candidate branch | fix/r18-usb-midi-ring-ownership |
| Final host-tested candidate SHA | **b800669caf209067c47447fb6376abfbe7eaa7b3** |
| Production patch completed at | 220a9d82b9e819b38e1bdb7232751bb1439f1fa3 |
| Successful host CI | [Run 42003303720](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42003303720), attempt 1, job 127163701513 |
| Archived GitHub host evidence | r18-usb-midi-host-source-evidence, artifact 13403441725, 30-day retention |
| Independent Delay candidate | fix/r18-delay-mono-safe-access at 08bc165500e21153eeb28ddbf2fe2d84626a82ca; **not merged** |

Exactly three production paths changed from the original R18: Core/Src/Soundengine/USB/usb_manager.c, Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c, and Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h. The test and CI additions are tests/r18_usb_midi/run.py and .github/workflows/r18-usb-midi-host.yml. CLP, mixer, FX bus, Delay, Compressor, main boot and all other production code are unchanged.

| Source | R18 Git blob | Candidate Git blob |
| --- | --- | --- |
| usb_manager.c | 6057ec247956382274a79772f793302a55edb1a2 | 7f30ed0fcdb433f280b98bb238879cb28834b585 |
| usbd_cdc.c | d193b9e9a554dc26a962960d71ec9c0d13e632d4 | ef33e3e2941db2a8a72705d66d419c29966f4d47 |
| usbd_cdc.h | 27cc529e8cd2a78cb8117017b0247ab096c0af26 | 56bb9165ae5d2bf0134a55a4a95652bc706aef77 |

## Actual faults and minimal implementation

**TX wrap:** Original usb_flushMidi computed a signed pointer difference in an unsigned eight-bit variable and passed it as a contiguous CDC transfer length. A read offset of 120 with a write offset of 4 could submit 140 bytes from offset 120, past the 128-byte queue. The candidate limits TX to aligned, contiguous blocks of at most 64 bytes and splits the wrap.

**TX busy and asynchronous ownership:** Original code advanced its read pointer whether CDC_Transmit_FS returned USBD_OK or USBD_BUSY, and did not retain the transmitted storage until completion. The candidate only marks accepted packets pending and advances the read pointer after the actual CDC TxState becomes idle. The existing class DataIn callback clears TxState only after final transfer completion, including an optional zero-length packet. BUSY/FAIL leaves data queued. During disconnection, pending state is cleared without advancing the queue, allowing retransmission after reconnect. **Exactly-once delivery across a near-complete disconnect is not guaranteed.**

**TX boundedness:** The existing 128-byte buffer stores complete four-byte USB-MIDI packets with one unused four-byte slot (maximum **31 queued events**). At capacity the *newest entire event* is rejected, preserving already queued or in-flight events; the volatile usb_MidiTxDropped counter records drops. This may still drop Note-Off or clock messages under overload.

**RX overwrite/full-as-empty:** Original USB DataOut could overwrite unread entries and after 64 queued events make the write index equal the read index, falsely indicating an empty ring. The candidate keeps one unused slot (maximum **63 queued messages**); it checks capacity before writing, drops a new complete event when full, and counts usb_MidiRxDropped. Read and write indices are volatile; producer/consumer publication includes DMB barriers. USB full-speed packet traversal is limited to complete four-byte events and the 64-byte FS endpoint packet boundary.

No MIDI event type mapping, hardware endpoint descriptor, kit preset, CLP signal path, or effect was deliberately changed. Overload handling is deliberately deterministic but still **lossy**: finite queues cannot promise lossless MIDI under indefinite overload.

## Actual tests, classifications and evidence

Final CI environment: Ubuntu 24.04; native GCC 13.3.0 at O1 with AddressSanitizer, UndefinedBehaviorSanitizer, bounds and alignment instrumentation. The test runner extracts production functions from the pinned R18 and candidate sources verbatim, checks all six source Git blob identities, and uses explicit host shims for CDC BUSY, TxState and USB DataOut. **Not Cortex-M7 hardware or real USB traffic.**

Executed:

    python3 tests/r18_usb_midi/run.py build/r18-usb-midi-ci

The final suite comprised **15 correctly classified process cases**, all with expected outcomes:

- Original normal TX and RX: PASS; candidate normal TX and RX bytes: **exact match**.
- Original TX BUSY: correctly reproduced data loss; original TX wrap: correctly reproduced invalid contiguous **140-byte** read from offset **120**.
- Original RX full: directly reproduced 64 entries making the queue appear empty; expected overflow-policy assertion failure also recorded.
- Candidate TX: normal; BUSY retry; other CDC traffic BUSY; persistent in-flight buffer protected while queue fills; wrap split to 8+4 bytes; disconnect/retry: **PASS**.
- Candidate RX: normal, 63 oldest held with 17 newest dropped on 80 events, queue drain/recovery, incomplete USB packet rejection: **PASS**.
- Existing R18 CLP and general regressions: **PASS**, including 75,000 mixer calculations, 135 core snapshots, 16,384 MAX route tests, 4,096 ramp checks, FX-bus reset, bug001, kit load, project storage and euclidean suites.
- Allowed-path and unchanged R18 CLP/Delay/Compressor checks: **PASS**.

CI evidence issue and correction history (all actual runs):

1. [42002576093](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42002576093): tests passed but archive checksums contained a self-reference; archive was not accepted as final verified evidence.
2. [42003075755](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42003075755): tests passed but CI evidence verification failed because the runner hashed a concurrently written terminal tee summary. This was an evidence-step failure, not a USB production failure.
3. **[42003303720](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42003303720): complete SUCCESS** after fixing the hashing procedure without changing the production patch.

The final artifact was downloaded and verified independently: **90,762 bytes**, SHA-256 **0b864ea92c55b582bf1bd5f7e9d2e6598d3b436b68f845b4a0823e8d3585f665**; ZIP CRC **PASS**, **40 files**, **17/17** runner file-hash entries and **35/35** CI-record-hash entries valid. Retained evidence includes generated source-extracted C, raw old-failure logs, corrected-case logs, test JSON, compiler info and regression logs. CI artifacts have only a 30-day retention, not permanent independent binary archival.

## Open limits and stop criteria

**NOT EXECUTED:** ARM cross-build, ELF/resource comparison, hardware MIDI host/DAW traffic, USB IRQ/DMA/cache stress, MCU audio deadlines, stack high-water, long-duration device soak, sound comparison on actual hardware, firmware installation or release. Additional counters and MIDI queue control may alter FLASH/RAM and foreground/ISR work; the impact is **unknown until an authorized ARM validation build**.

TX/RX saturation policy remains inherently lossy. USB disconnection retry is not exactly-once. Source-level DMB and volatile indexing are not a hardware concurrency certificate. Do not claim real-time deadline safety or guaranteed absence of stuck notes from host tests.

Recommended next step: independently approved **one ARM validation build from the exact USB candidate SHA**, followed by verification of compilation, section/memory deltas and an English per-build report. Keep accepted R18 untouched; do not automatically integrate the separate Delay fix, flash/install, merge/release, or begin Compressor modifications. Any hardware testing requires a separate owner-approved backup/recovery and test procedure.

## Immutable references

- [Final R18-to-USB diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/41b364067bce0d8cde624c184933d9cb88e2725b...b800669caf209067c47447fb6376abfbe7eaa7b3)
- [Reproducible USB host test source](https://github.com/DocLXR-02/lxr-desktop-app/blob/b800669caf209067c47447fb6376abfbe7eaa7b3/tests/r18_usb_midi/run.py)
- [Final USB host CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42003303720)
- [Accepted R18 sound and sources](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/CLP_R18_SONIC_ACCEPTANCE.md)

**Publication:** Separate report-only branch based on the project documentation branch. Canonical PROJECT_STATUS and private KnowledgeBase have **not** been modified by this work.
