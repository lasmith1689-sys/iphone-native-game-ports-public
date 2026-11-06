#ifndef PC64_MINIMAL_CPU64_V1_H
#define PC64_MINIMAL_CPU64_V1_H
#include <stdint.h>
#include <stddef.h>
#include "native_required.h"
typedef union {
    float    f32[4];
    double   f64[2];
    uint32_t u32[4];
    uint64_t u64[2];
    int32_t  i32[4];
    int64_t  i64[2];
    uint8_t  u8[16];
    /* The narrow lanes the packed-integer ops work in. A 32-bit target never
     * needed these because its compiler had x87 to convert with; a Win64 one
     * has only SSE, so pack/unpack/shift of bytes and words is everywhere. */
    int8_t   i8[16];
    uint16_t u16[8];
    int16_t  i16[8];
} XMM;
typedef struct {
    /* The sixteen general-purpose registers, in Intel encoding order so that
     * a register number from a REX-extended ModRM indexes this directly. */
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;
    /* Flags as discrete 0/1 bytes, as in the 32-bit runtime. */
    uint64_t cf, zf, sf, of, pf, af;
    XMM      xmm[16];
    uint32_t mxcsr;
} CPU;
static inline uint64_t rd64(uint64_t a){return pc64_required_read(a,8);}
static inline uint64_t pop64(CPU *c) { uint64_t v = rd64(c->rsp); c->rsp += 8; return v; }
void dispatch(CPU *c, uint64_t target);
void dispatch_jmp(CPU *c, uint64_t target);
_Static_assert(sizeof(CPU)==448&&_Alignof(CPU)==8,"native CPU ABI");
_Static_assert(offsetof(CPU,rip)==128&&offsetof(CPU,cf)==136&&offsetof(CPU,zf)==144&&offsetof(CPU,af)==176&&offsetof(CPU,xmm)==184&&offsetof(CPU,mxcsr)==440,"exact own field ABI");
#endif
