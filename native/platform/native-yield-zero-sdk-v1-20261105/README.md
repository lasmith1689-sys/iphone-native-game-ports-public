# Genuine SDK native scheduling yield

Three unchanged self-authored source files implement the native current-thread
yield policy. Windows invokes actual Sleep(0); Darwin invokes actual
sched_yield and reports-320 on nonzero return. This is a native scheduling
policy, not a fairness/elapsed-time/universal Win32 equivalence claim.

The driver requires genuine iPhoneOS sched.h and compiles the Darwin source
twice with complete object and stack-usage equality. The only support API is
pc_yield_zero_v1 and only unresolved symbol is sched_yield. Native ARM64 Mac
tests invoke the actual kernel-facing provider3072times using the original
thread and two distinct real pthreads, synchronized at a gate and joined
normally. Counts do not prove fairness, a particular context switch or real
time latency. This build does not force or certify the OS error path.

No recovered game source, asset, private import profile, save, key or signing
identity is included. No app or game is launched on an iPhone. Native Mac
test binaries can have normal linker ad-hoc signatures; no Apple development
identity or installable delivery is produced. Next: join exact provider ABI
to private native startup consumers. Same thread; no new thread required.
