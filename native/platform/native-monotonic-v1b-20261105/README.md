# Native elapsed-time support, version 1

Self-authored support only. The API returns a real native clock word, its
explicit domain and conversion ratio, and checked integer nanoseconds. The
64-by-32 product and 128-by-64 division retain the full intermediate value.
The division helper is an exact copy of our separately tested native word
arithmetic. It uses defined unsigned words without a 128-bit division runtime.

`monotonic.c` commits only a complete successful sample. Invalid output shape,
invalid ratio, provider failure and conversion overflow preserve the output.
Pointers must still designate live objects; alignment checks are not a memory
sandbox. Provider calls operate on a local candidate. There is no mutable
service-global state or cached clock frequency.

Windows uses the installed SDK's real QueryPerformanceFrequency/Counter.
Darwin uses the genuine installed SDK's mach_timebase_info/mach_absolute_time.
QPC and Mach absolute time have distinct origins and sleep behavior. The domain
tag remains part of the sample. Absolute timestamps from different domains
must never be subtracted or compared. This does not replace a game's TSC or
calibration/frequency policy: each source caller needs a separate admission.

Tests compare 65,914 exact products/divisions against compiler-supported
128-bit arithmetic, including zero ratios, full-width results and overflow.
Failure-provider tests check unchanged output, invalid domains and no provider
calls for rejected pointers. A separate production-provider executable reads
4,096 real native samples and verifies stable ratios and monotonicity during
that finite run. Synthetic provider tests never stand in for that live test.

The Apple build uses genuine headers, compiles every production source twice
for iPhoneOS, checks whole-object equality, and runs only this self-authored
test on the Mac host. It exports dependency identities rather than SDK header
contents. It does not run on an iPhone, sign an app, or execute game code.
App integration must supply Apple's required-reason declaration for elapsed
timing and prove the applicable foreground/suspend policy.

Primary references:
- https://developer.apple.com/documentation/kernel/mach_timebase_info_data_t
- https://developer.apple.com/documentation/driverkit/mach_absolute_time
- https://developer.apple.com/documentation/driverkit/mach_timebase_info-c.func
- https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
- https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps

Public scope: these own sources, synthetic tests and the bounded build driver
and workflow. No recovered game source, game identifiers/addresses, assets,
saves, keys or credentials are part of this support package.
