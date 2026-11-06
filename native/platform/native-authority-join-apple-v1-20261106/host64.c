#include "host64.h"
#define HOST_MAGIC 0x50433634u
typedef struct {uint64_t base,bytes;uint32_t access,reserved;} region;
typedef struct {
 uint32_t magic,count;
 region regions[64];
 uint64_t gs_base,gs_bytes,preferred,live,image_bytes;
 int64_t delta;
 pc64_cpuid_service cpuid;
 void *cpuid_context;
} host_state;
_Static_assert(sizeof(void *)==8,"Real native64 pointers required");
static host_state *host(void *p) {
 if(!p||((uintptr_t)p&7u))return 0;
 host_state *h=(host_state *)p;return h->magic==HOST_MAGIC?h:0;
}
static int span(uint64_t base,uint64_t size) {return base&&size&&size<=SIZE_MAX&&base<=UINT64_MAX-size;}
static int32_t allowed(host_state *h,uint64_t a,uint32_t n,uint32_t access) {
 if(!h)return PC64_PARAM;
 if(!n)return 0;
 if(a>UINT64_MAX-n)return PC64_OVERFLOW;
 for(uint32_t i=0;i<h->count;i++) {
  region *r=&h->regions[i];
  if(a>=r->base&&a-r->base<=r->bytes&&n<=r->bytes-(a-r->base))
   return (r->access&access)==access?0:PC64_PERMISSION;
 }
 return PC64_RANGE;
}
uint32_t pc64_host_size(void){return (uint32_t)sizeof(host_state);}
int32_t pc64_host_init(void *p,uint32_t bytes) {
 if(!p||((uintptr_t)p&7u)||bytes<sizeof(host_state))return PC64_PARAM;
 uint8_t *b=(uint8_t *)p;for(size_t i=0;i<sizeof(host_state);i++)b[i]=0;
 ((host_state *)p)->magic=HOST_MAGIC;return 0;
}
int32_t pc64_host_add_region(void *p,void *base,uint64_t bytes,uint32_t access) {
 host_state *h=host(p);uint64_t a=(uint64_t)(uintptr_t)base;
 if(!h||!span(a,bytes)||!access||(access&~3u))return PC64_PARAM;
 if(h->count==64)return PC64_CAPACITY;
 for(uint32_t i=0;i<h->count;i++) {
  region *r=&h->regions[i];if(a<r->base+r->bytes&&r->base<a+bytes)return PC64_PARAM;
 }
 region *r=&h->regions[h->count++];r->base=a;r->bytes=bytes;r->access=access;r->reserved=0;return 0;
}
int32_t pc64_host_set_gs(void *p,void *base,uint64_t bytes) {
 host_state *h=host(p);uint64_t a=(uint64_t)(uintptr_t)base;
 if(!h||!span(a,bytes))return PC64_PARAM;h->gs_base=a;h->gs_bytes=bytes;return 0;
}
int32_t pc64_host_set_cpuid(void *p,pc64_cpuid_service service,void *context) {
 host_state *h=host(p);if(!h)return PC64_PARAM;h->cpuid=service;h->cpuid_context=context;return 0;
}
int32_t pc64_host_cpuid(void *p,uint32_t leaf,uint32_t subleaf,uint32_t out[4]) {
 host_state *h=host(p);if(!h||!out)return PC64_PARAM;if(!h->cpuid)return PC64_UNSUPPORTED;
 uint32_t values[4];int32_t result=h->cpuid(h->cpuid_context,leaf,subleaf,values);
 if(result)return result;
 for(uint32_t i=0;i<4;i++)out[i]=values[i];return 0;
}
int32_t pc64_host_copy_from(void *p,uint64_t a,void *destination,uint32_t n) {
 if(n>16||(!destination&&n))return PC64_PARAM;
 int32_t result=allowed(host(p),a,n,PC64_READ);if(result)return result;
 uint8_t *d=(uint8_t *)destination;const uint8_t *s=(const uint8_t *)(uintptr_t)a;
 for(uint32_t i=0;i<n;i++)d[i]=s[i];return 0;
}
int32_t pc64_host_copy_to(void *p,uint64_t a,const void *source,uint32_t n) {
 if(n>16||(!source&&n))return PC64_PARAM;
 int32_t result=allowed(host(p),a,n,PC64_WRITE);if(result)return result;
 uint8_t *d=(uint8_t *)(uintptr_t)a;const uint8_t *s=(const uint8_t *)source;
 for(uint32_t i=0;i<n;i++)d[i]=s[i];return 0;
}
int32_t pc64_host_read(void *p,uint64_t a,uint32_t n,uint64_t *out) {
 if(!out||(n!=1&&n!=2&&n!=4&&n!=8))return PC64_PARAM;
 int32_t result=allowed(host(p),a,n,PC64_READ);if(result)return result;
 const uint8_t *b=(const uint8_t *)(uintptr_t)a;uint64_t value=0;
 for(uint32_t i=0;i<n;i++)value|=(uint64_t)b[i]<<(8*i);
 *out=value;return 0;
}
int32_t pc64_host_write(void *p,uint64_t a,uint32_t n,uint64_t value) {
 if(n!=1&&n!=2&&n!=4&&n!=8)return PC64_PARAM;
 int32_t result=allowed(host(p),a,n,PC64_WRITE);if(result)return result;
 uint8_t *b=(uint8_t *)(uintptr_t)a;
 for(uint32_t i=0;i<n;i++)b[i]=(uint8_t)(value>>(8*i));return 0;
}
int32_t pc64_host_gs_read(void *p,uint64_t offset,uint32_t n,uint64_t *out) {
 host_state *h=host(p);if(!h)return PC64_PARAM;if(!h->gs_base)return PC64_UNSUPPORTED;
 if(offset>h->gs_bytes||n>h->gs_bytes-offset)return PC64_RANGE;
 return pc64_host_read(h,h->gs_base+offset,n,out);
}
int32_t pc64_host_gs_write(void *p,uint64_t offset,uint32_t n,uint64_t value) {
 host_state *h=host(p);if(!h)return PC64_PARAM;if(!h->gs_base)return PC64_UNSUPPORTED;
 if(offset>h->gs_bytes||n>h->gs_bytes-offset)return PC64_RANGE;
 return pc64_host_write(h,h->gs_base+offset,n,value);
}
int32_t pc64_host_atomic(void *p,uint64_t a,uint32_t n,uint32_t op,uint64_t cmp,uint64_t value,uint64_t *out) {
 if(!out||(n!=4&&n!=8)||op<1||op>3)return PC64_PARAM;
 if(a&(n-1u))return PC64_ALIGNMENT;
 int32_t result=allowed(host(p),a,n,PC64_READ|PC64_WRITE);if(result)return result;
 if(n==4) {
  uint32_t *ptr=(uint32_t *)(uintptr_t)a,previous=(uint32_t)cmp;
  if(op==PC64_XADD)previous=__atomic_fetch_add(ptr,(uint32_t)value,__ATOMIC_SEQ_CST);
  else if(op==PC64_XCHG)previous=__atomic_exchange_n(ptr,(uint32_t)value,__ATOMIC_SEQ_CST);
  else (void)__atomic_compare_exchange_n(ptr,&previous,(uint32_t)value,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
  *out=previous;
 } else {
  uint64_t *ptr=(uint64_t *)(uintptr_t)a,previous=cmp;
  if(op==PC64_XADD)previous=__atomic_fetch_add(ptr,value,__ATOMIC_SEQ_CST);
  else if(op==PC64_XCHG)previous=__atomic_exchange_n(ptr,value,__ATOMIC_SEQ_CST);
  else (void)__atomic_compare_exchange_n(ptr,&previous,value,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
  *out=previous;
 }
 return 0;
}
int32_t pc64_host_set_image(void *p,uint64_t preferred,void *live,uint64_t bytes) {
 host_state *h=host(p);uint64_t a=(uint64_t)(uintptr_t)live;
 if(!h||!span(preferred,bytes)||!span(a,bytes))return PC64_PARAM;
 /* Reject ambiguous overlapping preferred/live ranges; identity is fine. */
 if(a!=preferred&&a<preferred+bytes&&preferred<a+bytes)return PC64_PARAM;
 uint64_t distance=a>=preferred?a-preferred:preferred-a;if(distance>INT64_MAX)return PC64_OVERFLOW;
 h->preferred=preferred;h->live=a;h->image_bytes=bytes;
 h->delta=a>=preferred?(int64_t)distance:-(int64_t)distance;return 0;
}
int32_t pc64_host_image_address(void *p,uint64_t address,uint64_t *out) {
 host_state *h=host(p);if(!h||!out)return PC64_PARAM;if(!h->image_bytes)return PC64_UNSUPPORTED;
 if(address<h->preferred||address-h->preferred>=h->image_bytes)return PC64_RANGE;
 *out=h->live+(address-h->preferred);return 0;
}
int32_t pc64_host_image_canonical(void *p,uint64_t address,uint64_t *out) {
 host_state *h=host(p);if(!h||!out)return PC64_PARAM;if(!h->image_bytes)return PC64_UNSUPPORTED;
 if(address>=h->preferred&&address-h->preferred<h->image_bytes){*out=address;return 0;}
 if(address>=h->live&&address-h->live<h->image_bytes){*out=h->preferred+(address-h->live);return 0;}
 return PC64_RANGE;
}
int32_t pc64_host_image_delta(void *p,int64_t *out) {
 host_state *h=host(p);if(!h||!out)return PC64_PARAM;if(!h->image_bytes)return PC64_UNSUPPORTED;
 *out=h->delta;return 0;
}
