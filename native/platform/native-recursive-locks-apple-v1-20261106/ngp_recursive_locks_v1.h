#ifndef NGP_RECURSIVE_LOCKS_V1_H
#define NGP_RECURSIVE_LOCKS_V1_H
#include "native_dispatch.h"
#include "ngp_recursive_lock_policy_v1.h"

enum {
 NGP_LOCK_ARGUMENT_V1=-380,NGP_LOCK_OWNER_V1=-381,NGP_LOCK_HOST_V1=-382,
 NGP_LOCK_FRAME_V1=-383,NGP_LOCK_STATE_V1=-384,NGP_LOCK_LIMIT_V1=-385,
 NGP_LOCK_PLATFORM_V1=-386,NGP_LOCK_RETAINED_V1=-387,NGP_LOCK_BUSY_V1=-388,
 NGP_LOCK_IDENTITY_V1=-389
};
enum {NGP_LOCK_EMPTY_V1=0,NGP_LOCK_CONFIGURED_V1=1,NGP_LOCK_CONSTRUCTING_V1=2,
 NGP_LOCK_LIVE_V1=3,NGP_LOCK_RETIRING_V1=4,NGP_LOCK_CLOSED_V1=5,NGP_LOCK_FAILED_V1=6};
enum {NGP_LOCK_CANCEL_V1=1,NGP_LOCK_ATTR_INIT_V1=2,NGP_LOCK_TYPE_V1=3,
 NGP_LOCK_MUTEX_INIT_V1=4,NGP_LOCK_ATTR_DESTROY_V1=5,NGP_LOCK_ENTER_V1=6,
 NGP_LOCK_LEAVE_V1=7,NGP_LOCK_MUTEX_DESTROY_V1=8,NGP_LOCK_CLEANUP_V1=9};
typedef struct {
 uint32_t version,size;uint64_t identity;uint32_t flags,reserved;uint64_t reserved2;
} NgpRecursiveLockDescriptorV1;
typedef struct {
 uint32_t version,size;int32_t status;uint32_t phase,worker_observation,slot_count;
 uint32_t attr_init_ack_mask,type_ack_mask,mutex_init_ack_mask,attr_destroy_ack_mask;
 uint32_t mutex_destroy_ack_mask,held_mask,uncertain_mask,max_held_depth,native_stage;
 int32_t native_error;uint64_t last_identity;uint32_t cancel_disable_ack,reserved;
 uint32_t secondary_native_stage;int32_t secondary_native_error;
} NgpRecursiveLocksReportV1;

/* OWN retained sidecars, not Windows bytes/imports, source record or readiness.
 * All native spans are valid/live/aligned/exclusive/disjoint: service, real
 * host/CPU, descriptor input, reports and stack. No pointer validation theorem.
 * Fresh zero storage/exact size; one canonical process attempt, no reset or
 * address/identity reuse. Immutable identities are copied keys, not pointers
 * authorized for dereference. Use no internal pthread objects externally.
 * Init is true nonmain, already-warmed sole cookie tag/actual host/depth0;
 * it disables cancellation, creates NO mutex and cannot report LIVE. Separate
 * attempt CAS precedes immutable initialization; release canonical publication
 * follows complete immutable fields/initial phase/cancellation outcome.
 * Construct/enter/leave require same actual owner/tag/host/CPU and positive
 * real dispatcher depth; all selected slots must be acknowledged before enter.
 * Sixteen slots/64 recursion; actual native lock/unlock once per ACK. Every
 * mutation is serialized by this retained worker, no async cancellation/dlclose.
 * Partial failure retains owners/cells to process exit, stops further native
 * use, preserves first cause and separate attribute-cleanup error; no retry.
 * Retire is depth0, no held ACKs and all construction ACKs; native destroy ACKs
 * are distinct from free/source callbacks/drain/G2. Storage always retained.
 * Reports contain own last-acknowledged tracking, never backend-live/quiescence
 * proof; FAILED masks/depth may lag actual effects. max_held_depth is historic
 * acknowledged peak. Foreign/observe fields are atomic phase only; zeros mean
 * unobserved, not no resources. Bad report shape/alias may leave output untouched.
 * CPU fields are never dereferenced or modified by this service. A later raw
 * source adapter must authenticate its source profiles and fault nonreturn on
 * failure before source ignores a BOOL/void result. No such adapter is here. */
PC64_API uint32_t ngp_recursive_locks_size_v1(void);
PC64_API uint32_t ngp_recursive_locks_alignment_v1(void);
PC64_API int32_t ngp_recursive_locks_init_v1(void*,uint32_t,void*,CPU*,const NgpRecursiveLockDescriptorV1*,uint32_t,NgpRecursiveLocksReportV1*);
PC64_API int32_t ngp_recursive_locks_construct_v1(void*,CPU*,uint64_t,NgpRecursiveLocksReportV1*);
PC64_API int32_t ngp_recursive_locks_enter_v1(void*,CPU*,uint64_t,NgpRecursiveLocksReportV1*);
PC64_API int32_t ngp_recursive_locks_leave_v1(void*,CPU*,uint64_t,NgpRecursiveLocksReportV1*);
PC64_API int32_t ngp_recursive_locks_retire_v1(void*,CPU*,NgpRecursiveLocksReportV1*);
PC64_API int32_t ngp_recursive_locks_observe_v1(void*,NgpRecursiveLocksReportV1*);
#ifdef NGP_LOCK_TESTING
typedef struct {uint32_t version,size,calls[10];} NgpRecursiveLocksTestStatsV1;
/* Testing replacement only: post-actual-API lost ACKs, no fake production
 * native API, allocator/tag/depth replacement or reset. */
PC64_API int32_t ngp_recursive_locks_test_fault_v1(void*,CPU*,uint64_t,uint32_t);
PC64_API int32_t ngp_recursive_locks_test_stats_v1(void*,CPU*,NgpRecursiveLocksTestStatsV1*);
#endif
#endif
