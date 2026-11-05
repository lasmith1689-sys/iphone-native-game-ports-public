#include "monotonic.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int32_t pc_clock_platform(pc_clock_sample *candidate) {
    LARGE_INTEGER frequency,counter;
    if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0 ||
       !QueryPerformanceCounter(&counter) || counter.QuadPart<0)return -2;
    candidate->ticks=(uint64_t)counter.QuadPart;
    candidate->numerator=1000000000;
    candidate->denominator=(uint64_t)frequency.QuadPart;
    candidate->domain=PC_CLOCK_WINDOWS_QPC;
    return 0;
}
