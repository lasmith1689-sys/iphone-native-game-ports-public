#include "virtual_counter.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
static unsigned checks,failures;
#define CHECK(x) do {checks++;if(!(x)){if(failures++<8)fprintf(stderr,"line%d %s\n",__LINE__,#x);}}while(0)
#if defined(PC_CLOCK_TEST_PROVIDER)
static unsigned calls;
static int mode;
int32_t pc_clock_platform(pc_clock_sample *s) {
    calls++;s->ticks=123456789;s->numerator=125;s->denominator=3;
    s->domain=PC_CLOCK_DARWIN_MACH_ABSOLUTE;
    if(mode==1)return -2;
    if(mode==2)s->numerator=0;
    if(mode==3)s->denominator=0;
    if(mode==4){s->ticks=UINT64_MAX;s->numerator=UINT32_MAX;s->denominator=1;}
    if(mode==5)s->domain=PC_CLOCK_WINDOWS_QPC;
    if(mode==6)s->numerator=124;
    if(mode==7)s->denominator=4;
    return 0;
}
#endif
int main(void) {
    pc_virtual_counter counter={0},zero={0},before;
    uint64_t result=UINT64_C(0x1234567812345678),sentinel=result;
    CHECK(sizeof counter==24 && _Alignof(pc_virtual_counter)==8);
    CHECK(pc_virtual_counter_read(&counter,&result)==-1 && result==sentinel);
    CHECK(pc_virtual_counter_frequency(&counter,&result)==-1 && result==sentinel);
    CHECK(pc_virtual_counter_init(0)==-1);
    _Alignas(8) unsigned char unaligned[64];memset(unaligned,0x7b,sizeof unaligned);
    unsigned char saved[64];memcpy(saved,unaligned,sizeof saved);
    CHECK(pc_virtual_counter_init((pc_virtual_counter *)(void *)(unaligned+1))==-1);
    CHECK(memcmp(saved,unaligned,sizeof saved)==0);
#if defined(PC_CLOCK_TEST_PROVIDER)
    CHECK(calls==0);
    for(mode=1;mode<=4;mode++) {
        CHECK(pc_virtual_counter_init(&counter)!=0);
        CHECK(memcmp(&counter,&zero,sizeof counter)==0);
    }
    mode=0;
#else
    (void)zero;
#endif
    CHECK(pc_virtual_counter_init(&counter)==0);before=counter;
    CHECK(pc_virtual_counter_init(&counter)==-1 && memcmp(&counter,&before,sizeof counter)==0);
    CHECK(pc_virtual_counter_frequency(&counter,&result)==0 && result==1000000000);
    result=sentinel;
    CHECK(pc_virtual_counter_read(&counter,0)==-1);
    CHECK(pc_virtual_counter_read(0,&result)==-1 && result==sentinel);
    CHECK(pc_virtual_counter_read(&counter,(uint64_t *)(void *)(unaligned+1))==-1);
    CHECK(memcmp(saved,unaligned,sizeof saved)==0);
    CHECK(pc_virtual_counter_read(&counter,&counter.guard)==-1);
    CHECK(pc_virtual_counter_frequency(&counter,&counter.native_denominator)==-1);
    CHECK(memcmp(&counter,&before,sizeof counter)==0);
#if defined(PC_CLOCK_TEST_PROVIDER)
    unsigned previous=calls;
    CHECK(pc_virtual_counter_frequency(&counter,&result)==0 && result==1000000000 && calls==previous);
    for(mode=1;mode<=7;mode++) {
        result=sentinel;CHECK(pc_virtual_counter_read(&counter,&result)!=0 && result==sentinel);
        CHECK(memcmp(&counter,&before,sizeof counter)==0);
    }
    mode=0;CHECK(pc_virtual_counter_read(&counter,&result)==0 && result==UINT64_C(5144032875));
    CHECK(calls==previous+8);
#else
    uint64_t first=0,prior=0;
    for(unsigned i=0;i<4096;i++) {
        CHECK(pc_virtual_counter_read(&counter,&result)==0);
        CHECK(result>=prior);if(!i)first=result;prior=result;
        CHECK(pc_virtual_counter_frequency(&counter,&result)==0 && result==1000000000);
    }
    CHECK(prior>first);
    printf("{\"nativeVirtualSamples\":4096,\"frequency\":1000000000,\"elapsedTicks\":%" PRIu64 "}\n",prior-first);
#endif
    /* Corrupted context cannot turn a real clock into a successful service. */
    counter.guard^=1;result=sentinel;
    CHECK(pc_virtual_counter_read(&counter,&result)==-1 && result==sentinel);
    CHECK(pc_virtual_counter_frequency(&counter,&result)==-1 && result==sentinel);
    printf("{\"checks\":%u,\"failures\":%u}\n",checks,failures);
    return failures?1:0;
}
