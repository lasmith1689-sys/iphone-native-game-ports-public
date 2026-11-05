/* Self-authored native elapsed-time service. No game ABI or x86 TSC promise.
 * All output pointers must designate live aligned objects of the stated type.
 * Status 0 commits output, -1 rejects shape/scale, 1 reports arithmetic
 * overflow, -2 reports platform/provider failure. Failure preserves output.
 * Origins, frequency and sleep behavior belong to the explicit clock domain.
 */
#ifndef PC_NATIVE_MONOTONIC_H
#define PC_NATIVE_MONOTONIC_H
#include <stdint.h>
#if defined(_WIN32)
#define PC_CLOCK_API __declspec(dllexport)
#else
#define PC_CLOCK_API __attribute__((visibility("default")))
#endif
enum { PC_CLOCK_WINDOWS_QPC=1, PC_CLOCK_DARWIN_MACH_ABSOLUTE=2 };
typedef struct pc_clock_sample {
    uint64_t ticks;
    uint64_t nanoseconds;
    uint64_t denominator;
    uint32_t numerator;
    uint32_t domain;
} pc_clock_sample;
PC_CLOCK_API int32_t pc_clock_scale(uint64_t ticks,uint32_t numerator,
                                    uint64_t denominator,uint64_t *nanoseconds);
PC_CLOCK_API int32_t pc_clock_read(pc_clock_sample *out);
/* Internal native provider; writes only its local candidate in pc_clock_read. */
int32_t pc_clock_platform(pc_clock_sample *candidate);
#endif
