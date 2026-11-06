#ifndef COOKIE_WORKER_TAG_V1_H
#define COOKIE_WORKER_TAG_V1_H
#include "worker_tag_frame_contract.h"
enum{COOKIE_TAG_UNBOUND_V1=-330,COOKIE_TAG_EXHAUSTED_V1=-331};
/* Own process-lifetime worker tag, not a Windows kernel thread ID. Native
 * scalar outputs must be valid aligned disjoint caller storage. Warm only at
 * depth0, then retain worker/TLS/provider lifetime. No reset/reuse API exists.
 * Repeated warm on the same live worker returns its same nonzero tag. */
PC64_API int32_t cookie_worker_tag_warm_v1(uint32_t*);
PC64_API int32_t cookie_worker_tag_read_v1(uint32_t*);
#endif
