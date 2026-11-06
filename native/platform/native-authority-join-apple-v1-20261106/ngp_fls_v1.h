#ifndef NGP_FLS_V1_H
#define NGP_FLS_V1_H
#include <stdint.h>
#include "ngp_fls_protocol_v1.h"
typedef int32_t (*NgpFlsOwnCallbackV1)(void *context,void *value);
typedef struct {NgpFlsOwnCallbackV1 function;void *context;} NgpFlsCallbackV1;
typedef struct {
 uint32_t version,size;int32_t status;uint32_t phase,worker_observation;
 uint32_t token,boxes_used,key_created,value_clear_ack,callback_required;
 uint32_t callback_attempted,callback_ack,key_delete_ack,owner_exit_observed;
 int32_t native_error;
} NgpFlsReportV1;
/* G1 OWN protocol only: no Windows/raw/guest record or cleanup callback.
 * Prepare is called once from actual native main BEFORE starting workers;
 * Apple also verifies pthread_main_np. Windows main identity is source-bound
 * construction, not a portable OS main-thread detector or ready Boolean.
 * Storage starts zeroed, exact size/alignment, live/disjoint from host,
 * descriptor/report/output/value objects. Actual host and linked-once tag
 * support remain live. Init and explicit retirement require depth0; all
 * mutation uses the same retained nonmain worker. Init disables cancellation;
 * do not re-enable it, externally delete/use the private key or dlclose code.
 * Every published service/64 embedded boxes persists to process exit.
 * Caller retains callback code/context and ALL assigned values until proved
 * callback retirement; replace/NULL does not invoke or free prior values.
 * Each nonnull set consumes a fresh box even for an identical value; no reuse.
 * Scalar outputs preserved on refusal; bad shape/alias admission can leave
 * reports untouched. Other admitted errors publish status. Foreign
 * reports read only immutable fields/atomics, not mutable counts or key state.
 * Observe is always atomic-only, valid even after owner exit; zero diagnostic
 * counts mean unobserved, not no resources. key_created is historical create
 * acknowledgement, not actual key-live state. No exit flag is liveness proof.
 * FAILED tracking may lag real partial effects: never free/retry/quiescence.
 * No pointer validation or pthread identity-reuse theorem is provided. */
NGP_FLS_API int32_t ngp_fls_prepare_process_v1(void);
NGP_FLS_API uint32_t ngp_fls_size_v1(void);
NGP_FLS_API uint32_t ngp_fls_alignment_v1(void);
NGP_FLS_API int32_t ngp_fls_init_v1(void*,uint32_t,void*,const NgpFlsCallbackV1*,NgpFlsReportV1*);
NGP_FLS_API int32_t ngp_fls_allocate_v1(void*,uint32_t*,NgpFlsReportV1*);
NGP_FLS_API int32_t ngp_fls_get_v1(void*,uint32_t,void**,NgpFlsReportV1*);
NGP_FLS_API int32_t ngp_fls_set_v1(void*,uint32_t,void*,NgpFlsReportV1*);
NGP_FLS_API int32_t ngp_fls_retire_v1(void*,uint32_t,NgpFlsReportV1*);
NGP_FLS_API int32_t ngp_fls_observe_v1(void*,NgpFlsReportV1*);
#ifdef NGP_FLS_TESTING
/* Explicit test-only post-actual-API uncertainty and late-event injection.
 * No production global reset, fake key, or replacement native API exists. */
NGP_FLS_API int32_t ngp_fls_test_fault_v1(void*,uint32_t);
NGP_FLS_API int32_t ngp_fls_test_late_exit_v1(void*);
#endif
#endif
