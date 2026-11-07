/* Own first-cause publication. No device/engine authority or readiness value. */
#ifndef NGP_BOOT_FAILURE_V1_H
#define NGP_BOOT_FAILURE_V1_H
#include <stdint.h>
#include <stdatomic.h>
typedef struct {uint32_t stage;int32_t status;} NgpBootFailureV1;
typedef struct {_Atomic unsigned publication;NgpBootFailureV1 cause;} NgpBootFailureLatchV1;
/* Initialize before publishing the retained caller to any other thread. */
void ngp_boot_failure_init_v1(NgpBootFailureLatchV1 *);
/* Concurrent writers, first valid nonzero-status attempt wins. The winning
 * writer publishes immutable fields; no callbacks or allocation in between. */
int ngp_boot_failure_record_v1(NgpBootFailureLatchV1 *,uint32_t stage,int32_t status);
/* Nonblocking read:0 means absent/in-progress, output untouched;1 means copied
 * immutable first cause. No wait/spin or reset/retry/destruction authority. */
int ngp_boot_failure_copy_v1(const NgpBootFailureLatchV1 *,NgpBootFailureV1 *);
#endif
