#include "monotonic.h"
#include <mach/mach_time.h>
int32_t pc_clock_platform(pc_clock_sample *candidate) {
    mach_timebase_info_data_t timebase;
    if(mach_timebase_info(&timebase)!=KERN_SUCCESS || !timebase.numer || !timebase.denom)return -2;
    candidate->ticks=mach_absolute_time();
    candidate->numerator=timebase.numer;
    candidate->denominator=timebase.denom;
    candidate->domain=PC_CLOCK_DARWIN_MACH_ABSOLUTE;
    return 0;
}
