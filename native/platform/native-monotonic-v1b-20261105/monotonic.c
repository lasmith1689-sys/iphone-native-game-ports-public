#include "monotonic.h"
#include "div64.h"
_Static_assert(sizeof(pc_clock_sample)==32,"sample ABI");
_Static_assert(_Alignof(pc_clock_sample)==8,"sample alignment");

int32_t pc_clock_scale(uint64_t ticks,uint32_t numerator,uint64_t denominator,uint64_t *out) {
    if(!out || ((uintptr_t)out&7) || !numerator || !denominator)return -1;
    /* Exact 64x32 product in unsigned limbs; no overflowing signed operation,
       floating-point rounding, or compiler 128-bit division runtime. */
    uint64_t p0=(ticks&UINT64_C(0xffffffff))*numerator;
    uint64_t p1=(ticks>>32)*numerator;
    uint64_t low=p0+(p1<<32),high=(p1>>32)+(uint64_t)(low<p0);
    uint64_t quotient,remainder;
    int32_t status=pc64_udiv64_bits(high,low,denominator,&quotient,&remainder);
    if(status)return status;
    *out=quotient;return 0;
}

int32_t pc_clock_read(pc_clock_sample *out) {
    if(!out || ((uintptr_t)out&7))return -1;
    pc_clock_sample candidate={0};
    if(pc_clock_platform(&candidate))return -2;
    if(candidate.domain!=PC_CLOCK_WINDOWS_QPC &&
       candidate.domain!=PC_CLOCK_DARWIN_MACH_ABSOLUTE)return -2;
    int32_t status=pc_clock_scale(candidate.ticks,candidate.numerator,
                                candidate.denominator,&candidate.nanoseconds);
    if(status)return status;
    *out=candidate;return 0;
}
