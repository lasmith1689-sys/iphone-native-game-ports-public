#include "yield_zero.h"
#if defined(SD_DARWIN_SDK0_PROBE)
#if SD_DARWIN_SDK0_PROBE != 1 || !defined(__APPLE__) || !defined(__aarch64__)
#error Explicit SDK0 declaration probe is only for Apple ARM64
#endif
extern int sched_yield(void);
#else
#include <sched.h>
#endif
int32_t pc_yield_zero_v1(void){return sched_yield()==0?0:PC_NATIVE_YIELD_FAILED_V1;}
