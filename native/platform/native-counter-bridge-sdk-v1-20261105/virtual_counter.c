#include "virtual_counter.h"
#define COUNTER_GUARD UINT64_C(0x504356434f554e54)
_Static_assert(sizeof(pc_virtual_counter)==24,"counter ABI");
_Static_assert(_Alignof(pc_virtual_counter)==8,"counter alignment");
static int shape(const pc_virtual_counter *c) {
    return c && !((uintptr_t)c&7) && (uintptr_t)c<=UINTPTR_MAX-sizeof(*c);
}
static int valid(const pc_virtual_counter *c) {
    return shape(c) && c->guard==COUNTER_GUARD && c->native_numerator &&
      c->native_denominator && (c->native_domain==PC_CLOCK_WINDOWS_QPC ||
                               c->native_domain==PC_CLOCK_DARWIN_MACH_ABSOLUTE);
}
static int output(const pc_virtual_counter *c,uint64_t *out) {
    if(!out || ((uintptr_t)out&7) || (uintptr_t)out>UINTPTR_MAX-sizeof(*out))return 0;
    uintptr_t begin=(uintptr_t)c,p=(uintptr_t)out;
    return p+sizeof(*out)<=begin || begin+sizeof(*c)<=p;
}
int32_t pc_virtual_counter_init(pc_virtual_counter *c) {
    if(!shape(c) || c->guard || c->native_denominator || c->native_numerator || c->native_domain)return -1;
    pc_clock_sample sample;int32_t status=pc_clock_read(&sample);
    if(status)return status;
    pc_virtual_counter candidate={COUNTER_GUARD,sample.denominator,sample.numerator,sample.domain};
    *c=candidate;return 0;
}
int32_t pc_virtual_counter_read(const pc_virtual_counter *c,uint64_t *out) {
    if(!valid(c) || !output(c,out))return -1;
    pc_clock_sample sample;int32_t status=pc_clock_read(&sample);
    if(status)return status;
    if(sample.domain!=c->native_domain || sample.numerator!=c->native_numerator ||
       sample.denominator!=c->native_denominator)return -2;
    *out=sample.nanoseconds;return 0;
}
int32_t pc_virtual_counter_frequency(const pc_virtual_counter *c,uint64_t *out) {
    if(!valid(c) || !output(c,out))return -1;
    *out=UINT64_C(1000000000);return 0;
}
