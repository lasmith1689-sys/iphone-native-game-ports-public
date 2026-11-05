#include "div64.h"

static int outputs(uint64_t *q,uint64_t *r) {
    return q && r && q!=r && !((uintptr_t)q&7) && !((uintptr_t)r&7);
}

/* high<divisor proves the unsigned quotient fits in64 bits. The carry bit
 * keeps the65-bit intermediate remainder exact using defined unsigned words. */
static void divide_magnitude(uint64_t high,uint64_t low,uint64_t divisor,uint64_t *q,uint64_t *r) {
    uint64_t quotient=0,remainder=high;
    for(uint32_t i=64;i-- >0;) {
        uint64_t carry=remainder>>63;
        remainder=(remainder<<1)|((low>>i)&1);
        if(carry || remainder>=divisor) {
            remainder-=divisor;
            quotient|=UINT64_C(1)<<i;
        }
    }
    *q=quotient;*r=remainder;
}

int32_t pc64_udiv64_bits(uint64_t high,uint64_t low,uint64_t divisor,uint64_t *q,uint64_t *r) {
    if(!outputs(q,r))return -1;
    if(!divisor || high>=divisor)return 1;
    uint64_t quotient,remainder;
    divide_magnitude(high,low,divisor,&quotient,&remainder);
    *q=quotient;*r=remainder;return 0;
}

int32_t pc64_sdiv64_bits(uint64_t high,uint64_t low,uint64_t divisor,uint64_t *q,uint64_t *r) {
    if(!outputs(q,r))return -1;
    if(!divisor)return 1;
    uint64_t negative_n=high>>63,negative_d=divisor>>63;
    uint64_t magnitude_low=low,magnitude_high=high;
    if(negative_n) {
        magnitude_low=UINT64_C(0)-low;
        magnitude_high=~high+(uint64_t)(magnitude_low==0);
    }
    uint64_t magnitude_d=negative_d?UINT64_C(0)-divisor:divisor;
    if(magnitude_high>=magnitude_d)return 1;
    uint64_t quotient,remainder;
    divide_magnitude(magnitude_high,magnitude_low,magnitude_d,&quotient,&remainder);
    uint64_t negative_q=negative_n^negative_d;
    if(quotient>(negative_q?UINT64_C(0x8000000000000000):UINT64_C(0x7fffffffffffffff)))return 1;
    *q=negative_q?UINT64_C(0)-quotient:quotient;
    *r=negative_n?UINT64_C(0)-remainder:remainder;
    return 0;
}
