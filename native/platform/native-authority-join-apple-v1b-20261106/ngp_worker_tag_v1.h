#ifndef NGP_WORKER_TAG_V1_H
#define NGP_WORKER_TAG_V1_H
#include "ngp_fls_environment_v1.h"
enum{NGP_WORKER_TAG_UNBOUND_V1=-330,NGP_WORKER_TAG_EXHAUSTED_V1=-331};
/* Own process-lifetime worker tag, not a Windows kernel thread ID. Native
 * scalar outputs must be valid aligned disjoint caller storage. Warm only at
 * depth0, then retain worker/TLS/provider lifetime. No reset/reuse API exists.
 * Repeated warm on the same live worker returns its same nonzero tag. */
NGP_FLS_API int32_t ngp_worker_tag_warm_v1(uint32_t*);
NGP_FLS_API int32_t ngp_worker_tag_read_v1(uint32_t*);
#endif
