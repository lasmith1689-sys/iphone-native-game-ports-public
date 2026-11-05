#ifndef PC_NATIVE_YIELD_ZERO_V1_H
#define PC_NATIVE_YIELD_ZERO_V1_H
#include <stdint.h>
enum {PC_NATIVE_YIELD_FAILED_V1=-320};
/* Invoke the current native thread's scheduling-yield policy once. Zero means
 * the actual platform invocation returned normally; not a fairness, elapsed
 * time or universal Windows scheduling-equivalence guarantee. */
int32_t pc_yield_zero_v1(void);
#endif
