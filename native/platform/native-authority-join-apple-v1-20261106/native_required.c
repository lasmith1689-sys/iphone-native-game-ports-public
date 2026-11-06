#include "native_required.h"
#include <stdlib.h>
static _Thread_local void *active;
static _Thread_local pc64_fault_handler on_fault;
static _Thread_local void *fault_context;
void pc64_bind_host(void *host,pc64_fault_handler handler,void *context) {
 active=host;on_fault=handler;fault_context=context;
}
void *pc64_active_host(void){return active;}
_Noreturn void pc64_fail(int32_t status,uint64_t address,uint64_t detail) {
 if(on_fault)on_fault(fault_context,status,address,detail);
 abort(); /* Returning fault callbacks are a contract violation. */
}
uint64_t pc64_required_read(uint64_t a,uint32_t n) {
 uint64_t value;int32_t s=pc64_host_read(active,a,n,&value);if(s)pc64_fail(s,a,n);return value;
}
void pc64_required_write(uint64_t a,uint32_t n,uint64_t value) {
 int32_t s=pc64_host_write(active,a,n,value);if(s)pc64_fail(s,a,n);
}
void pc64_required_copy_from(uint64_t a,void *dst,uint32_t n) {
 int32_t s=pc64_host_copy_from(active,a,dst,n);if(s)pc64_fail(s,a,n);
}
void pc64_required_copy_to(uint64_t a,const void *src,uint32_t n) {
 int32_t s=pc64_host_copy_to(active,a,src,n);if(s)pc64_fail(s,a,n);
}
uint64_t pc64_required_gs_read(uint64_t off,uint32_t n) {
 uint64_t value;int32_t s=pc64_host_gs_read(active,off,n,&value);if(s)pc64_fail(s,off,n);return value;
}
void pc64_required_gs_write(uint64_t off,uint32_t n,uint64_t value) {
 int32_t s=pc64_host_gs_write(active,off,n,value);if(s)pc64_fail(s,off,n);
}
uint64_t pc64_required_atomic(uint64_t a,uint32_t n,uint32_t op,uint64_t cmp,uint64_t value) {
 uint64_t previous;int32_t s=pc64_host_atomic(active,a,n,op,cmp,value,&previous);
 if(s)pc64_fail(s,a,n);return previous;
}
void pc64_required_cpuid(uint32_t leaf,uint32_t subleaf,uint32_t out[4]) {
 int32_t s=pc64_host_cpuid(active,leaf,subleaf,out);if(s)pc64_fail(s,leaf,subleaf);
}
uint64_t pc64_required_image_address(uint64_t preferred) {
 uint64_t address;int32_t s=pc64_host_image_address(active,preferred,&address);
 if(s)pc64_fail(s,preferred,0);return address;
}
