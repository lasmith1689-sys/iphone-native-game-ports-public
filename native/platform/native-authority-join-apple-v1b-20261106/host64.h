#ifndef PCRECOMP_NATIVE_HOST64_H
#define PCRECOMP_NATIVE_HOST64_H
#include <stdint.h>
#include <stddef.h>
#if defined(_WIN32)
#define PC64_API __declspec(dllexport)
#else
#define PC64_API __attribute__((visibility("default")))
#endif
enum pc64_status {PC64_OK=0,PC64_PARAM=-1,PC64_RANGE=-2,PC64_PERMISSION=-3,
 PC64_UNSUPPORTED=-4,PC64_ALIGNMENT=-5,PC64_OVERFLOW=-6,PC64_CAPACITY=-7};
enum pc64_access {PC64_READ=1,PC64_WRITE=2};
enum pc64_atomic_operation {PC64_XADD=1,PC64_CMPXCHG=2,PC64_XCHG=3};
typedef int32_t (*pc64_cpuid_service)(void *context,uint32_t leaf,uint32_t subleaf,uint32_t out[4]);
/* New explicit native host obligations, not a Windows API/CPU interpreter.
 * All states start zeroed, 8-byte aligned, live and disjoint from writable
 * operands. Registered addresses are REAL host pointers at full64-bit width.
 * Caller registers stable native image/heap/stack/TLS memory and serializes
 * registration/lifecycle. Atomic operands must be naturally aligned and all
 * concurrent accesses obey C atomic/data-race rules. GS bytes/CPUID profiles
 * are initialized by the host; absence never fabricates zero/success.
 */
PC64_API uint32_t pc64_host_size(void);
PC64_API int32_t pc64_host_init(void *host,uint32_t bytes);
PC64_API int32_t pc64_host_add_region(void *host,void *base,uint64_t bytes,uint32_t permissions);
PC64_API int32_t pc64_host_set_gs(void *host,void *base,uint64_t bytes);
PC64_API int32_t pc64_host_set_cpuid(void *host,pc64_cpuid_service service,void *context);
PC64_API int32_t pc64_host_cpuid(void *host,uint32_t leaf,uint32_t subleaf,uint32_t out[4]);
PC64_API int32_t pc64_host_read(void *host,uint64_t address,uint32_t bytes,uint64_t *out);
PC64_API int32_t pc64_host_write(void *host,uint64_t address,uint32_t bytes,uint64_t value);
PC64_API int32_t pc64_host_copy_from(void *host,uint64_t address,void *destination,uint32_t bytes);
PC64_API int32_t pc64_host_copy_to(void *host,uint64_t address,const void *source,uint32_t bytes);
PC64_API int32_t pc64_host_gs_read(void *host,uint64_t offset,uint32_t bytes,uint64_t *out);
PC64_API int32_t pc64_host_gs_write(void *host,uint64_t offset,uint32_t bytes,uint64_t value);
PC64_API int32_t pc64_host_atomic(void *host,uint64_t address,uint32_t width,
 uint32_t operation,uint64_t expected,uint64_t value,uint64_t *previous);
PC64_API int32_t pc64_host_set_image(void *host,uint64_t preferred,void *live,uint64_t bytes);
PC64_API int32_t pc64_host_image_address(void *host,uint64_t preferred_address,uint64_t *live_address);
PC64_API int32_t pc64_host_image_canonical(void *host,uint64_t code_address,uint64_t *preferred_address);
PC64_API int32_t pc64_host_image_delta(void *host,int64_t *delta);
#endif
