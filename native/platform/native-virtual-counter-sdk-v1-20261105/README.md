# Native virtual counter policy

This self-authored service defines one virtual tick as one integer nanosecond.
Its frequency is exactly 1,000,000,000 units per second. This describes the
unit, not hardware resolution, accuracy or physical processor cycles.
Counter reads and frequency queries use this same definition.

Initialization samples our genuine native clock and records its actual domain
and conversion ratio in a typed 24-byte context. The caller starts the context
with `{0}`, initializes it once, publishes it with a real happens-before
relationship, and retains it immutable through every reader. The service
has no global mutable state and allocates no storage. Reinitialization and
uninitialized/corrupt contexts are rejected. Native ratio/domain changes,
platform errors and nanosecond overflow preserve the entire caller output.
Outputs that overlap the context, null or misaligned pointers are rejected;
all pointers still require valid live backing. Construction and lifetime are
owner obligations, not a memory sandbox or arbitrary-thread cleanup proof.

The underlying clock is the frozen native elapsed-time implementation:
Windows QueryPerformanceCounter/Frequency or Darwin mach_absolute_time with
its real mach_timebase_info ratio. Its 96-bit product is divided without
intermediate truncation. QPC and Mach domains have different origins and
sleep behavior; a context refuses to cross domains. No wall-clock or FILETIME
value is produced here. Frequency queries do not sample the clock.

Native fixture RED failed 17 semantic checks before implementation. GREEN
passes 44 checks for initialization, output/context preservation, alias/shape
refusal, provider failure/overflow and changed metadata. A separate Windows
fixture passes 12,307 checks with 4,096 real virtual-counter reads; each
frequency query returns the exact declared virtual rate. SDK builds also run
these fixtures on a real Mac with actual Mach services, and compile the
production sources twice against the genuine iPhone SDK.

This is a deliberate native porting policy, not an assertion that a source
program's physical TSC or QPC already had this frequency. Source opcode and
import bindings must explicitly use these units together, prove their
calling/return and storage contracts, and handle foreground/suspend behavior.
Those game bindings and a physical iPhone run are separate unfinished work.
No game source, addresses, retail assets, keys or signing credentials are in
this support package.

Clock API references:
- https://developer.apple.com/documentation/kernel/mach_timebase_info_data_t
- https://developer.apple.com/documentation/driverkit/mach_absolute_time
- https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
- https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
