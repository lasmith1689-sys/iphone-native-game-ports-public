#ifndef PC64_NATIVE_REQUIRED_H
#define PC64_NATIVE_REQUIRED_H
#include "host64.h"
typedef void (*pc64_fault_handler)(void *context,int32_t status,uint64_t address,uint64_t detail);
/* TLS context must outlive every translated call on its thread. Fault
 * handlers must not return (a host-owned real setjmp landing pad is valid).
 * No handler means native abort, never invented CPU/OS success. */
PC64_API void pc64_bind_host(void *host,pc64_fault_handler handler,void *context);
PC64_API void *pc64_active_host(void);
_Noreturn void pc64_fail(int32_t status,uint64_t address,uint64_t detail);
uint64_t pc64_required_read(uint64_t address,uint32_t bytes);
void pc64_required_write(uint64_t address,uint32_t bytes,uint64_t value);
void pc64_required_copy_from(uint64_t address,void *destination,uint32_t bytes);
void pc64_required_copy_to(uint64_t address,const void *source,uint32_t bytes);
uint64_t pc64_required_gs_read(uint64_t offset,uint32_t bytes);
void pc64_required_gs_write(uint64_t offset,uint32_t bytes,uint64_t value);
uint64_t pc64_required_atomic(uint64_t address,uint32_t bytes,uint32_t operation,uint64_t expected,uint64_t value);
void pc64_required_cpuid(uint32_t leaf,uint32_t subleaf,uint32_t out[4]);
uint64_t pc64_required_image_address(uint64_t preferred_address);
#endif
