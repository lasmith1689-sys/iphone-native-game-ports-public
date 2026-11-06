#include "cookie_worker_tag.h"
#include "cookie_tag_policy.h"
#include <stdatomic.h>
static _Atomic uint32_t last_tag;
static _Thread_local uint32_t worker_tag;
static int output(uint32_t*p){uintptr_t a=(uintptr_t)p,b=(uintptr_t)&worker_tag,c=(uintptr_t)&last_tag;return a&&!(a&3)&&a<=UINTPTR_MAX-4&&(a+4<=b||b+4<=a)&&(a+4<=c||c+sizeof(last_tag)<=a);}
int32_t cookie_worker_tag_warm_v1(uint32_t*out){
 if(!output(out))return PC64_PARAM;if(pc64_dispatch_depth()!=0)return PC64_COUNTER_FRAME_V1;
 if(!worker_tag){uint32_t old=atomic_load_explicit(&last_tag,memory_order_relaxed),next;
  do{if(!cookie_tag_next_v1(old,UINT32_MAX,&next))return COOKIE_TAG_EXHAUSTED_V1;}while(!atomic_compare_exchange_weak_explicit(&last_tag,&old,next,memory_order_relaxed,memory_order_relaxed));worker_tag=next;}
 *out=worker_tag;return 0;
}
int32_t cookie_worker_tag_read_v1(uint32_t*out){if(!output(out))return PC64_PARAM;if(!worker_tag)return COOKIE_TAG_UNBOUND_V1;*out=worker_tag;return 0;}
