/* TEST ONLY: explicit per-thread controlled platform samples, not OS time. */
#ifndef PC64_COUNTER_TEST_PROVIDER
#error Controlled provider is excluded from production
#endif
#include "monotonic.h"
typedef struct {uint64_t ticks;uint32_t domain,numerator;uint64_t denominator;int32_t failure;uint32_t calls;} pc64_test_counter_sample;
_Thread_local pc64_test_counter_sample pc64_test_counter={0,PC_CLOCK_WINDOWS_QPC,1,1,0,0};
int32_t pc_clock_platform(pc_clock_sample*out)
{pc64_test_counter.calls++;out->ticks=pc64_test_counter.ticks;out->numerator=pc64_test_counter.numerator;out->denominator=pc64_test_counter.denominator;out->domain=pc64_test_counter.domain;return pc64_test_counter.failure;}
