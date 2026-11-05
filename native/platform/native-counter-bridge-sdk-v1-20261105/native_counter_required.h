#ifndef PC64_NATIVE_COUNTER_REQUIRED_V1_H
#define PC64_NATIVE_COUNTER_REQUIRED_V1_H
#include "native_dispatch.h"
#include "virtual_counter.h"
enum {PC64_COUNTER_UNBOUND_V1=-300,PC64_COUNTER_HOST_V1=-301,PC64_COUNTER_PROVIDER_V1=-302,PC64_COUNTER_CONTEXT_V1=-303,PC64_COUNTER_FRAME_V1=-304,PC64_COUNTER_WORKER_V1=-305,PC64_COUNTER_BUSY_V1=-306};
/* Opaque zeroed8-aligned one-shot context. Init copies the initialized typed
 * counter/domain by value and captures the genuine typed read function. The
 * retained creating pthread, immutable context/provider/code, live host and
 * disjoint valid CPU/guest/context memory are caller obligations. Publication
 * and lifetime changes are serialized; no forced cancellation or async use.
 * All control calls require native depth0. Unbind after ALL frames returned
 * or an outer real landing pad stopped/discarded the invocation; depth0 is
 * an observed gate, not a whole-producer quiescence proof.
 */
PC64_API uint32_t pc64_counter_binding_size_v1(void);
PC64_API int32_t pc64_counter_binding_init_v1(void *,uint32_t,void *,const pc_virtual_counter *);
PC64_API int32_t pc64_counter_bind_v1(void *);
PC64_API int32_t pc64_counter_unbind_after_stop_v1(void);
/* Soft reads share the EXACT active retained binding. They are valid inside
 * native frames and write only the explicit owned non-CPU scalar output on0.
 * Failure preserves output; output must be8-aligned/live and disjoint from
 * the complete binding. Binding/input errors or exact provider statuses are
 * returned; these functions never stamp CPU state or call pc64_fail.
 * Frequency is obtained from pc_virtual_counter_frequency on the snapshot.
 */
PC64_API int32_t pc64_counter_read_active_v1(uint64_t *ticks);
PC64_API int32_t pc64_counter_frequency_active_v1(uint64_t *frequency);
/* Integer nanosecond ticks only. Success changes exactly RAX/RDX, each
 * zero-extended from32bits. Fault changes only RIP, then never returns.
 * Provider failure detail is its exact int32 status encoded as uint32.
 * Caller must stop/discard faulted state; no guest unwind/resume is supplied.
 */
PC64_API void pc64_required_counter(CPU *cpu,uint64_t sourceVA);
#endif
