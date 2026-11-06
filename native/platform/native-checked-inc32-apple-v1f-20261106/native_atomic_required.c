#include "native_atomic_required.h"
#include "inc32_publish.h"
#include "cookie_worker_tag.h"
#include <stddef.h>
#include <pthread.h>
#include <stdatomic.h>
#define MAGIC UINT64_C(0x5043494e43333231)
enum{READY=1,BOUND=2,TERMINAL=3};
typedef struct{
 uint64_t magic;void*host;CPU*cpu;pthread_t worker;uint32_t tag,count;
 _Atomic uint32_t phase;Pc64IncSiteV1 sites[9];
}Context;
static _Thread_local Context*active;
_Static_assert(sizeof(Pc64IncSiteV1)==32&&_Alignof(Pc64IncSiteV1)==8,"exact descriptor ABI");
_Static_assert(sizeof(CPU)==448&&sizeof(uint32_t)==4&&ATOMIC_INT_LOCK_FREE==2,"CPU/DWORD/phase ABI");
#if __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
#error actual native little-endian memory required
#endif
static int span(const void*p,size_t n,size_t align){uintptr_t a=(uintptr_t)p;return a&&!(a%align)&&n&&a<=UINTPTR_MAX-n;}
static int overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<y+m&&y<x+n;}
static int32_t owner(Context*s){
 if(!span(s,sizeof*s,_Alignof(Context))||s->magic!=MAGIC)return PC64_INC_CONTEXT;
 uint32_t tag=0;if(cookie_worker_tag_read_v1(&tag)||tag!=s->tag)return PC64_INC_WORKER;
 if(!pthread_equal(pthread_self(),s->worker)||pc64_active_host()!=s->host)return PC64_INC_WORKER;
 return 0;
}
static int32_t opcode_shape(const Pc64IncSiteV1*d){
 if(!d->sourceVA||!d->expectedRIP||d->operandKind<1||d->operandKind>4)return PC64_INC_SITE;
 static const uint8_t first[4][7]={{0xf0,0xff,0},{0xf0,0xff,1},{0xf0,0xff,2},{0xf0,0xff,0x80,0x5c,1,0,0}};
 uint32_t n=d->operandKind==PC64_INC_RAX_15C?7:3;if(d->byteCount!=n||d->sourceVA>UINT64_MAX-n)return PC64_INC_SITE;
 for(uint32_t i=0;i<8;i++)if(d->originalBytes[i]!=(i<n?first[d->operandKind-1][i]:0))return PC64_INC_SITE;
 return 0;
}
static int32_t authenticate(void*host,const Pc64IncSiteV1*d,uint64_t*at){
 uint64_t address=0;*at=d->sourceVA;int32_t q=pc64_host_image_address(host,d->sourceVA,&address);if(q)return q;*at=address;
 uint8_t bytes[8];q=pc64_host_copy_from(host,address,bytes,d->byteCount);if(q)return q;
 for(uint32_t i=0;i<d->byteCount;i++)if(bytes[i]!=d->originalBytes[i])return PC64_INC_IMAGE;
 return 0;
}
uint32_t pc64_inc_binding_size_v1(void){return (uint32_t)sizeof(Context);}
uint32_t pc64_inc_binding_alignment_v1(void){return (uint32_t)_Alignof(Context);}
int32_t pc64_inc_binding_init_v1(void*storage,uint32_t bytes,void*host,CPU*cpu,const Pc64IncSiteV1*descriptors,uint32_t count){
 Context*s=storage;if(bytes!=sizeof*s||!span(s,sizeof*s,_Alignof(Context))||!span(host,pc64_host_size(),8)||!span(cpu,sizeof*cpu,_Alignof(CPU))||count<1||count>9||!span(descriptors,count*sizeof*descriptors,_Alignof(Pc64IncSiteV1)))return PC64_INC_CONTEXT;
 const void*p[4]={s,host,cpu,descriptors};size_t n[4]={sizeof*s,pc64_host_size(),sizeof*cpu,count*sizeof*descriptors};for(unsigned i=0;i<4;i++)for(unsigned j=i+1;j<4;j++)if(overlap(p[i],n[i],p[j],n[j]))return PC64_INC_ALIAS;
 if(pc64_dispatch_depth()!=0||active)return PC64_INC_STATE;if(pc64_active_host()!=host)return PC64_INC_WORKER;
#if defined(__APPLE__)
 if(pthread_main_np())return PC64_INC_WORKER;
#endif
 for(size_t i=0;i<sizeof*s;i++)if(((const uint8_t*)s)[i])return PC64_INC_CONTEXT;
 for(uint32_t i=0;i<count;i++){
  const Pc64IncSiteV1*d=&descriptors[i];int32_t q=opcode_shape(d);if(q)return q;
  if(i&&descriptors[i-1].sourceVA+descriptors[i-1].byteCount>d->sourceVA)return PC64_INC_SITE;
  uint64_t at=0;q=authenticate(host,d,&at);if(q)return q;
  for(unsigned z=0;z<4;z++)if(overlap((void*)(uintptr_t)at,d->byteCount,p[z],n[z]))return PC64_INC_ALIAS;
  q=pc64_host_image_address(host,d->expectedRIP,&at);if(q)return q;
 }
 uint32_t tag=0;int32_t q=cookie_worker_tag_warm_v1(&tag);if(q)return q;if(!tag)return PC64_INC_WORKER;
 s->host=host;s->cpu=cpu;s->worker=pthread_self();s->tag=tag;s->count=count;for(uint32_t i=0;i<count;i++)s->sites[i]=descriptors[i];atomic_init(&s->phase,READY);s->magic=MAGIC;return 0;
}
int32_t pc64_inc_bind_v1(void*storage){Context*s=storage;int32_t q=owner(s);if(q)return q;if(pc64_dispatch_depth()!=0||active||atomic_load_explicit(&s->phase,memory_order_acquire)!=READY)return PC64_INC_STATE;atomic_store_explicit(&s->phase,BOUND,memory_order_release);active=s;return 0;}
int32_t pc64_inc_unbind_after_stop_v1(void){Context*s=active;if(!s)return 0;int32_t q=owner(s);if(q)return q;if(pc64_dispatch_depth()!=0)return PC64_INC_STATE;atomic_store_explicit(&s->phase,TERMINAL,memory_order_release);active=NULL;return 0;}
static _Noreturn void actor_fail(int32_t q,uint64_t va){pc64_fail(q,va,0);}
static _Noreturn void source_fail(CPU*cpu,int32_t q,uint64_t va,uint64_t address,uint64_t width){cpu->rip=va;pc64_fail(q,address,width);}
void pc64_required_inc32_v1(CPU*cpu,uint64_t address,uint64_t sourceVA){
 Context*s=active;if(!s)actor_fail(PC64_INC_CONTEXT,sourceVA);int32_t q=owner(s);if(q)actor_fail(q,sourceVA);
 if(cpu!=s->cpu)actor_fail(PC64_INC_CPU,sourceVA);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)!=BOUND||!pc64_dispatch_depth())actor_fail(PC64_INC_STATE,sourceVA);
 const Pc64IncSiteV1*d=NULL;for(uint32_t i=0;i<s->count;i++)if(s->sites[i].sourceVA==sourceVA){d=&s->sites[i];break;}
 if(!d||cpu->rip!=d->expectedRIP)source_fail(cpu,PC64_INC_SITE,sourceVA,sourceVA,0);
 uint64_t wanted=d->operandKind==PC64_INC_RCX?cpu->rcx:d->operandKind==PC64_INC_RDX?cpu->rdx:cpu->rax+(d->operandKind==PC64_INC_RAX_15C?UINT64_C(0x15c):0);
 if(address!=wanted)source_fail(cpu,PC64_INC_SITE,sourceVA,address,4);
 if(address<=UINT64_MAX-4&&(overlap((void*)(uintptr_t)address,4,s,sizeof*s)||overlap((void*)(uintptr_t)address,4,s->cpu,sizeof(CPU))||overlap((void*)(uintptr_t)address,4,s->host,pc64_host_size())))source_fail(cpu,PC64_INC_ALIAS,sourceVA,address,4);
 for(uint32_t i=0;i<s->count;i++){
  uint64_t live=0;q=pc64_host_image_address(s->host,s->sites[i].sourceVA,&live);if(q)source_fail(cpu,q,sourceVA,s->sites[i].sourceVA,s->sites[i].byteCount);
  if(address<=UINT64_MAX-4&&overlap((void*)(uintptr_t)address,4,(void*)(uintptr_t)live,s->sites[i].byteCount))source_fail(cpu,PC64_INC_ALIAS,sourceVA,address,4);
 }
 uint64_t at=0;q=authenticate(s->host,d,&at);if(q)source_fail(cpu,q,sourceVA,at,d->byteCount);
 uint64_t old=0;q=pc64_host_atomic(s->host,address,4,PC64_XADD,0,1,&old);if(q)source_fail(cpu,q,sourceVA,address,4);
 pc64_inc32_publish_v1(cpu,old);
}
