#include "monotonic.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

static uint64_t state=UINT64_C(0x738dc4421a4e5599);
static uint64_t next_word(void) {
    state^=state<<13;state^=state>>7;state^=state<<17;return state;
}
static unsigned cases,failures;
#define CHECK(expr) do {cases++;if(!(expr)) {if(failures++<8) \
    fprintf(stderr,"line %d: %s\n",__LINE__,#expr);}}while(0)

#if defined(PC_CLOCK_TEST_PROVIDER)
static int provider_mode,provider_calls;
int32_t pc_clock_platform(pc_clock_sample *s) {
    provider_calls++;
    s->ticks=UINT64_MAX;s->denominator=100;s->numerator=3;
    s->domain=PC_CLOCK_DARWIN_MACH_ABSOLUTE;
    if(provider_mode==1)return -2;
    if(provider_mode==2)s->denominator=0;
    if(provider_mode==3){s->numerator=UINT32_MAX;s->denominator=1;}
    if(provider_mode==4)s->domain=99;
    return 0;
}
#endif

static void check_scale(uint64_t ticks,uint32_t n,uint64_t d) {
    uint64_t result=UINT64_C(0xabad1dea33334444),before=result;
    int32_t expected;
    __uint128_t quotient=0;
    if(!n || !d)expected=-1;
    else {quotient=(__uint128_t)ticks*n/d;expected=quotient>UINT64_MAX?1:0;}
    int32_t got=pc_clock_scale(ticks,n,d,&result);
    CHECK(got==expected);
    CHECK(result==(expected==0?(uint64_t)quotient:before));
}
int main(void) {
    const uint64_t ticks[]={0,1,UINT32_MAX,UINT64_C(1)<<32,
        UINT64_C(1)<<63,UINT64_MAX-1,UINT64_MAX};
    const uint32_t nums[]={0,1,3,125,1000000000,UINT32_MAX};
    const uint64_t dens[]={0,1,3,24,10000000,UINT32_MAX,
        UINT64_C(1)<<32,UINT64_C(1)<<63,UINT64_MAX};
    for(unsigned i=0;i<sizeof(ticks)/sizeof(*ticks);i++)
        for(unsigned j=0;j<sizeof(nums)/sizeof(*nums);j++)
            for(unsigned k=0;k<sizeof(dens)/sizeof(*dens);k++)
                check_scale(ticks[i],nums[j],dens[k]);
    for(unsigned i=0;i<65536;i++) {
        uint64_t t=next_word(),d=next_word();uint32_t n=(uint32_t)next_word();
        if(i%5==0)d=(uint32_t)d;
        if(i%257==0)d=0;
        if(i%263==0)n=0;
        check_scale(t,n,d);
    }
    CHECK(pc_clock_scale(1,1,1,0)==-1);
    _Alignas(8) unsigned char unaligned[48];memset(unaligned,0x6b,sizeof unaligned);
    unsigned char prior[48];memcpy(prior,unaligned,sizeof prior);
    CHECK(pc_clock_scale(1,1,1,(uint64_t *)(void *)(unaligned+1))==-1);
    CHECK(memcmp(prior,unaligned,sizeof prior)==0);
    CHECK(pc_clock_read(0)==-1);
    CHECK(pc_clock_read((pc_clock_sample *)(void *)(unaligned+1))==-1);
    CHECK(memcmp(prior,unaligned,sizeof prior)==0);
#if defined(PC_CLOCK_TEST_PROVIDER)
    CHECK(provider_calls==0);
    pc_clock_sample out,before;memset(&out,0x73,sizeof out);before=out;
    for(provider_mode=1;provider_mode<=4;provider_mode++) {
        int32_t expected=provider_mode==2?-1:provider_mode==3?1:-2;
        CHECK(pc_clock_read(&out)==expected);
        CHECK(memcmp(&out,&before,sizeof out)==0);
    }
    provider_mode=0;CHECK(pc_clock_read(&out)==0);
    CHECK(out.ticks==UINT64_MAX && out.nanoseconds==UINT64_C(553402322211286548));
    CHECK(out.domain==PC_CLOCK_DARWIN_MACH_ABSOLUTE && out.numerator==3 && out.denominator==100);
#else
    pc_clock_sample previous={0},current;
    uint64_t first=0;
    for(unsigned i=0;i<4096;i++) {
        CHECK(pc_clock_read(&current)==0);
        CHECK(current.numerator>0 && current.denominator>0);
        CHECK(current.domain==PC_CLOCK_WINDOWS_QPC || current.domain==PC_CLOCK_DARWIN_MACH_ABSOLUTE);
        uint64_t scaled=0;
        CHECK(pc_clock_scale(current.ticks,current.numerator,current.denominator,&scaled)==0);
        CHECK(scaled==current.nanoseconds);
        if(i) {
            CHECK(current.domain==previous.domain && current.numerator==previous.numerator && current.denominator==previous.denominator);
            CHECK(current.ticks>=previous.ticks && current.nanoseconds>=previous.nanoseconds);
        } else first=current.nanoseconds;
        previous=current;
    }
    CHECK(current.nanoseconds>first);
    printf("{\"domain\":%u,\"numerator\":%u,\"denominator\":%" PRIu64 ",\"nativeSamples\":4096,\"elapsedNanoseconds\":%" PRIu64 "}\n",
           current.domain,current.numerator,current.denominator,current.nanoseconds-first);
#endif
    printf("{\"checks\":%u,\"failures\":%u,\"scaleVectors\":65914}\n",cases,failures);
    return failures?1:0;
}
