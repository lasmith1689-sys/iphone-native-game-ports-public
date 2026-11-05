/* Own native word semantics for x86 DIV/IDIV64. No guest exception delivery.
 * Inputs are bit words. q/r must be live, aligned, disjoint uint64_t objects.
 * 0 commits both words; 1 is divide-by-zero/quotient overflow; -1 bad outputs.
 * All failure statuses preserve both outputs. Flags and source ABI are outside.
 */
#ifndef PC64_DIV64_H
#define PC64_DIV64_H
#include <stdint.h>
#if defined(_WIN32)
#define PC64_DIV64_API __declspec(dllexport)
#else
#define PC64_DIV64_API __attribute__((visibility("default")))
#endif
PC64_DIV64_API int32_t pc64_udiv64_bits(uint64_t high,uint64_t low,uint64_t divisor,uint64_t *q,uint64_t *r);
PC64_DIV64_API int32_t pc64_sdiv64_bits(uint64_t high,uint64_t low,uint64_t divisor,uint64_t *q,uint64_t *r);
#endif
