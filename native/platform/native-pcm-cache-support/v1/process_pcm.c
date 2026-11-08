#include "process_pcm.h"
#define PCM_LIMIT UINT32_C(67108864)
static uint64_t nearest_even(uint64_t n, uint32_t d) {
    uint64_t q=n/d, r=n%d;
    return q+(r*2>d || (r*2==d && (q&1)));
}
static int extent(const void *p, size_t n) {
    return p && n && n<=UINTPTR_MAX-(uintptr_t)p;
}
static int overlap(const void *a, size_t an, const void *b, size_t bn) {
    return (uintptr_t)a<(uintptr_t)b+bn && (uintptr_t)b<(uintptr_t)a+an;
}
/* Explicit x86 low-32 arithmetic and arithmetic right shift, no signed UB. */
static int32_t sar(uint32_t x, unsigned n) {
    if (x&UINT32_C(0x80000000)) return -1-(int32_t)((~x)>>n);
    return (int32_t)(x>>n);
}
static uint32_t sample(const unsigned char *p, uint32_t bits) {
    if (bits==8) return (uint32_t)((int32_t)p[0]-128);
    uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8);
    return x&32768 ? x|UINT32_C(0xffff0000) : x;
}
static void emit(unsigned char *p, uint32_t a, uint32_t bits) {
    int32_t x=sar(a,11);
    if (x<-32768) x=-32768;
    if (x>32767) x=32767;
    if (bits==8) p[0]=(unsigned char)(sar((uint32_t)x,8)+128);
    else {p[0]=(unsigned char)(uint32_t)x; p[1]=(unsigned char)((uint32_t)x>>8);}
}
int cod_miles_process_pcm_v1(const unsigned char *source, size_t source_bytes,
    const uint32_t record[30], uint32_t target_rate, uint32_t target_format,
    unsigned char *output, size_t capacity, cod_miles_process_result_v1 *result) {
    if (!extent(source,source_bytes) || source_bytes>PCM_LIMIT ||
        !extent(record,120) || (uintptr_t)record%_Alignof(uint32_t) ||
        !extent(output,capacity) || capacity>PCM_LIMIT ||
        !extent(result,sizeof(*result)) || (uintptr_t)result%_Alignof(cod_miles_process_result_v1)) return 1;
    if (overlap(output,capacity,source,source_bytes) || overlap(output,capacity,record,120) ||
        overlap(result,sizeof(*result),source,source_bytes) || overlap(result,sizeof(*result),record,120) ||
        overlap(output,capacity,result,sizeof(*result))) return 1;
    uint32_t bits=record[4], channels=record[5], rate=record[3];
    if (record[0]!=1 || (bits!=8 && bits!=16) || (channels!=1 && channels!=2) ||
        rate<8000 || rate>192000 || target_rate<8000 || target_rate>192000 || target_format>3 ||
        record[2]!=source_bytes || record[1]!=record[8] || !record[1] ||
        source_bytes>UINT32_MAX-record[1]) return 2;
    uint32_t unit=bits/8, align=unit*channels;
    if (source_bytes%align || record[6]!=source_bytes/unit || record[7]!=align) return 2;
    uint32_t frames=(uint32_t)source_bytes/align, outchannels=1+((target_format&2)!=0);
    uint32_t outunit=1+(target_format&1), outalign=outunit*outchannels;
    uint64_t required=nearest_even((uint64_t)frames*target_rate,rate)*outalign+256;
    if (required!=capacity) return 3;
    /* No refusal after this point: all source/output extents and operations bounded. */
    uint32_t pos=0, phase=0, previous[2]={0,0}, written=0, step=(uint32_t)nearest_even((uint64_t)rate*65536,target_rate);
    uint32_t initialized=0;
    for (unsigned i=0;i<30;i++) result->record[i]=record[i];
    for (;;) {
        uint32_t todo=(uint32_t)nearest_even((uint64_t)(frames-pos)*target_rate,rate);
        uint32_t room=((uint32_t)capacity-written)/outalign;
        if (todo>room) todo=room;
        if (!todo) break;
        if (!initialized || !pos) {
            result->record[9]=align; result->record[10]=0; result->record[11]=0;
            phase=0; previous[0]=0; previous[1]=0; initialized=1;
        }
        uint32_t actual=0;
        while (todo) {
            uint32_t wanted=todo, quantum=2048/outchannels;
            if (wanted>quantum) wanted=quantum;
            actual=0;
            for (;actual<wanted;actual++) {
                if (pos>=frames) break;
                uint32_t acc[2]={0,0};
                if (rate==target_rate) {
                    for (uint32_t c=0;c<channels;c++) acc[c]=sample(source+(size_t)pos*align+c*unit,bits)<<(27-bits);
                    pos++;
                } else if (step<=65536) {
                    uint32_t current[2]={0,0};
                    for (uint32_t c=0;c<channels;c++) {
                        current[c]=sample(source+(size_t)pos*align+c*unit,bits);
                        acc[c]=(previous[c]+(uint32_t)sar((current[c]-previous[c])*phase,16))<<(27-bits);
                    }
                    /* Original uses only step's fractional part in this branch,
                     * including the unequal-rate case that rounds to 65536. */
                    phase+=step&65535;
                    if (phase>=65536) {phase-=65536;pos++;previous[0]=current[0];previous[1]=current[1];}
                } else {
                    uint32_t partial=phase&UINT32_C(0xc0000000), sums[2]={previous[0],previous[1]};
                    phase&=UINT32_C(0x3fffffff);
                    if (!partial) {
                        for (uint32_t c=0;c<channels;c++) sums[c]=(uint32_t)sar(previous[c]*(65536-phase),16);
                        phase+=step;
                    }
                    if (!(partial&UINT32_C(0x40000000)) || (partial&UINT32_C(0x80000000))) {
                        while (phase>=131072 && pos<frames) {
                            for (uint32_t c=0;c<channels;c++) sums[c]+=sample(source+(size_t)pos*align+c*unit,bits);
                            pos++;phase-=65536;
                        }
                        if (phase>=131072) {phase|=UINT32_C(0xc0000000);previous[0]=sums[0];previous[1]=sums[1];break;}
                        phase&=65535;
                    }
                    if (pos>=frames) {phase|=UINT32_C(0x40000000);previous[0]=sums[0];previous[1]=sums[1];break;}
                    uint32_t scale=(uint32_t)((UINT64_C(1)<<(43-bits))/step);
                    for (uint32_t c=0;c<channels;c++) {
                        previous[c]=sample(source+(size_t)pos*align+c*unit,bits);
                        acc[c]=(sums[c]+(uint32_t)sar(previous[c]*phase,16))*scale;
                    }
                    pos++;
                }
                unsigned char *destination=output+(size_t)written+(size_t)actual*outalign;
                if (outchannels==1) emit(destination,acc[0]+(channels==2?acc[1]:0),outunit*8);
                else {emit(destination,acc[0],outunit*8);emit(destination+outunit,acc[channels==2?1:0],outunit*8);}
            }
            if (!actual) break;
            /* Original advances by the requested quantum, even if merge ended
             * early. Unwritten bytes retain the caller's allocation contents. */
            written+=wanted*outalign;todo-=wanted;
        }
        if (!actual) break;
    }
    result->written_bytes=written;
    result->record[1]+=pos*align;result->record[2]-=pos*align;
    if (initialized) {result->record[27]=phase;result->record[28]=previous[0];result->record[29]=previous[1];}
    return 0;
}
