# Cooperative retained-stop wait: complete own component candidate

This self-authored support component adds a worker-only bounded retained-stop
wait to the real voice/runtime and actual iPhone broker. It preserves the old
64-byte stop report and enum values, appends WAIT9, retains all native objects,
and does not supply source guest entry, startup, G2/free/restart or a game.

The real provider uses clock_gettime(CLOCK_MONOTONIC), nanosleep and errno.
Deadline is fixed5e9ns, waits are positive<=2e6ns, with4096claimed observation
cycles. The final reserved token remains valid through its query/fresh-clock/
ACKcommit or wait; exhaustion cannot create a4097th operation. STOPPED commits
only after fresh post-query time strictly before the deadline. First wait fault
quarantines the runtime before cleanup and remains sticky in later old polls.
Queue admission uses the actual canonical release/acquire publication, immutable
queue identity, actual control-specific key and service/epoch. No caller-ready
boolean, ACTIVE property, event pump or main/control queue wait is introduced.

The planned genuine iPhoneSDK27.1/arm64-ios26 cohort is all11production TUs,
primary/repeat22objects, strict unsigned classic-fixup linked libraries and raw
objects/.d/.su/headers/tool identities. No test provider or diagnostic fixture
belongs to that image. Existing20sources derive from a reviewed own baseline;
five existing files change and two real OS-provider files are added. All whole
source changes are reviewed privately before this own-source public projection.

Planned Mac evidence uses real MA voice/runtime, with explicitly synthetic
brokers. Deterministic clock replacement tests34primary modes plus an explicit
one-literal zero-budget source variant. The variant may perform real close and
one request before refusing, but performs no query/sleep/commit; production has
no runtime budget injection. A separate provider-body unit substitutes only
clock/sleep syscalls to test timespec arithmetic/errno/interval refusal. A third
smoke links the genuine OS provider and actual worker/voice backend, observing
real monotonic samples and one bounded sleep path, while the broker stays
synthetic. Earlier-worker-error tests skip clock entirely; no later unqueried
errno is claimed observed. Actual UIKit/ObjC admission runtime is not tested on
Mac: those rows have source and genuine iPhoneSDK evidence only.

All20old retained-stop outcomes and three old-API parent counterexamples are
retained. The old nonwaiting negatives intentionally remain negatives; genuine
NEW-API semantic REDs were separately observed against an explicit test-only
naive adapter before writing this implementation. That adapter is excluded from
this payload and all production links.

The complete planned capture is40objects,10strictlinks,60fresh-process Mac
outcomes and146command records, including26empty-stdout preflights. Failure
diagnostics and every bounded raw output are retained. Stream caps are observed
soft limits, not a disk-write or escaped-descendant theorem. Compiler .su rows
are local frame evidence, not decoded prologues or whole-stack bounds. SDK/tool
bytes are not copied as files; failing diagnostic logs can contain SDK snippets.
The exact public miniaudio source/LICENSE are fetched at pinned commit/hash.

This is component source/build evidence, not actual iPhone timing, physical
audio, game/provider-stack admission, signing, installation or full-engine
integration. No retail/recovered source, assets, private phone/build metadata,
profiles, passwords, secrets or keys are included. Actual first outcomes must be
retained honestly; fixes use fresh namespaces/jobs, never consumed reruns.

Next: execute the reviewed frozen dedicated feature workflow, retain/replay its
first artifact, then independent root whole-candidate review before any
integration. Continue current thread; no new thread is required.
