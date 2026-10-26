# Phase C USB class storage regression

Run `python3 tests/usb_allocator/run.py` using native GCC, Python 3 and nm.
ASan/UBSan must be available. No hardware, USB device, target register, flashing
utility or network access is used. Temporary outputs are outside the repository.

The runner extracts unchanged production bodies at runtime: class init/release/
deinit and buffer setters; application Init/DeInit callbacks; core configuration,
reset, disconnect and stop; and standard SET_CONFIGURATION handling. It uses real
project USB/STM32 headers. LL endpoint open/close/receive, stop and control responses
are explicit host mocks. USB heap aliases abort if called. The complete class TU
is also compiled and its undefined symbols checked for allocator dependencies.

Coverage includes 2,000 simulated FS/HS lifecycle cycles with duplicate config,
deconfig, reset, disconnect/reconnect, stop/restart, stable pointer/typed alignment,
complete zeroing on fresh configuration/cleanup, foreign ownership, callback
re-entry, malformed inputs and fourteen rollback/retention/retry failure scenarios.
Errors are injected at all three opens/closes, application init/deinit and receive
preparation, including compound cleanup failures and missing application buffers.
These are source regressions, not physical USB enumeration or packet tests.

Production owns one typed `USBD_CDC_HandleTypeDef` static object for the single
registered device/class. Its target size is 540 bytes, alignment 4; host pointer
width makes it 552 bytes/alignment 8. The typed object prevents size/alignment drift.
One owner pointer and two byte flags track lifetime and nonblocking re-entry.
Existing core lifecycle dispatch is serialized in the USB context; the byte flag
is not a mutex or a cross-context lifecycle lock. Application buffer callbacks
remain unchanged and use hUsbDeviceFS, matching the registered single FS device.

A direct duplicate Init fails before touching live endpoints/state. The USB core
still ACKs repeated SET_CONFIGURATION with the same value without calling Init.
DeInit with no active owner and NULL class data is idempotent. Foreign/invalid
pointers are never freed. Cleanup closes the fixed three endpoints before calling
application DeInit, then clears pClassData and storage and releases ownership.
Failed endpoint/app cleanup retains storage and ownership; Init rejects reuse until
a later DeInit/reset can finish. Core functions that ignore a class cleanup return
are unchanged: a host reset/deconfig may be needed after failure. Permanent HAL
failure is not claimed to recover physically. No allocator or unbounded wait is
used by this correction.

Descriptors, endpoints, class registration and all class code following lifecycle
functions remain unchanged. Global USBD_malloc/free aliases remain definitions but
have no compiled class users. Adding another class/device or an asynchronous
lifecycle caller requires ownership and allocator review. Data APIs retain their
existing configured-class preconditions; this patch is not a complete USB data-path
concurrency redesign. It removes the identified USB-vs-formatting allocator path,
not all possible future library reentrancy risks.

Run the Bug 001, Euclidean and safety suites as well. Build normal and diagnostic
Release/Debug and validate all images with tests/euclidean/check_images.py.
LXR_RUNTIME_DIAGNOSTICS remains OFF by default. No image is uploaded by these tests.
