/* Own native timing policy. One virtual tick is one integer nanosecond;
 * 1GHz describes units, never hardware resolution or physical CPU cycles.
 * Context starts {0}; owner publishes it after successful initialization,
 * retains it immutable through all readers, and serializes initialization.
 * Live typed objects and external lifetime/happens-before rules are required.
 * Status0 commits output; any failure preserves context and output.
 */
#ifndef PC_VIRTUAL_COUNTER_H
#define PC_VIRTUAL_COUNTER_H
#include "monotonic.h"
typedef struct pc_virtual_counter {
    uint64_t guard;
    uint64_t native_denominator;
    uint32_t native_numerator;
    uint32_t native_domain;
} pc_virtual_counter;
PC_CLOCK_API int32_t pc_virtual_counter_init(pc_virtual_counter *counter);
PC_CLOCK_API int32_t pc_virtual_counter_read(const pc_virtual_counter *counter,uint64_t *ticks);
PC_CLOCK_API int32_t pc_virtual_counter_frequency(const pc_virtual_counter *counter,uint64_t *frequency);
#endif
