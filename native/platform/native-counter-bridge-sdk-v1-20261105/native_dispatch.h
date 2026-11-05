#ifndef PC64_NATIVE_DISPATCH_H
#define PC64_NATIVE_DISPATCH_H
#include "cpu64.h"
typedef void (*pc64_native_function)(CPU *cpu);
typedef struct {uint64_t preferred_address;pc64_native_function function;} pc64_native_entry;
typedef int32_t (*pc64_native_import)(void *context,CPU *cpu,uint32_t index);
enum pc64_dispatch_error {PC64_DISPATCH_BUDGET=-100,PC64_DISPATCH_DEPTH=-101,PC64_DISPATCH_CONTRACT=-102};
/* Native compiled-function registry, no instruction fetch/decode/interpreter.
 * Caller owns immutable sorted entries containing real compiled function
 * pointers, not casts from guest addresses. Context is per-thread; external
 * lifecycle synchronization and valid disjoint host storage are required.
 * Import callback updates declared CPU outputs but MUST leave RSP unchanged;
 * a normal successful import then consumes one guest return word here.
 * Unknown services are errors, never forwarded to Windows DLLs.
 * Import sentinel window must be reserved and disjoint from every live/
 * preferred image and memory span; the loader owns that namespace contract.
 * Host image bindings and registry entries cannot change while installed.
 */
PC64_API uint32_t pc64_dispatch_size(void);
PC64_API int32_t pc64_dispatch_init(void *context,uint32_t bytes,void *host,
 const pc64_native_entry *entries,uint32_t count,uint64_t import_base,uint32_t import_count,
 pc64_native_import service,void *service_context);
PC64_API int32_t pc64_dispatch_bind(void *context,uint64_t native_entry_budget);
/* After all native frames have returned OR an outer fault landing pad has
 * stopped/discarded the invocation. This does not implement guest unwind or
 * permit resuming faulted guest state. */
PC64_API void pc64_dispatch_unbind_after_stop(void);
PC64_API uint64_t pc64_dispatch_entries_remaining(void);
PC64_API uint32_t pc64_dispatch_depth(void);
/* Before a generated function's local _ind switch: normalize live-image
 * code addresses to preferred labels; leave nonimage sentinel values for
 * the explicit dispatcher/import boundary to validate. */
PC64_API uint64_t pc64_canonical_indirect(uint64_t address);
int es3_cs_depth_get(void);
void es3_cs_depth_set(int depth);
#endif
