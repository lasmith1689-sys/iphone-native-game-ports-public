#include "ngp_worker_tag_v1.h"
#include "cookie_worker_tag.h"
_Static_assert((int32_t)NGP_FLS_ENV_ARGUMENT==(int32_t)PC64_PARAM,"same argument status");
_Static_assert((int32_t)NGP_FLS_ENV_FRAME==(int32_t)PC64_COUNTER_FRAME_V1,"same real depth refusal");
_Static_assert((int32_t)NGP_WORKER_TAG_UNBOUND_V1==(int32_t)COOKIE_TAG_UNBOUND_V1,"same unbound status");
_Static_assert((int32_t)NGP_WORKER_TAG_EXHAUSTED_V1==(int32_t)COOKIE_TAG_EXHAUSTED_V1,"same saturation status");
int32_t ngp_worker_tag_warm_v1(uint32_t*out){return cookie_worker_tag_warm_v1(out);}
int32_t ngp_worker_tag_read_v1(uint32_t*out){return cookie_worker_tag_read_v1(out);}
