#include "native_counter_required.h"
#if defined(SD_DARWIN_SDK0_PROBE)
#if SD_DARWIN_SDK0_PROBE != 1 || !defined(__APPLE__) || !defined(__aarch64__)
#error Selected SDK0 declaration probe only for Apple ARM64
#endif
typedef struct _opaque_pthread_t *CounterThread;
extern CounterThread pthread_self(void);
extern int pthread_equal(CounterThread,CounterThread);
#else
#include <pthread.h>
typedef pthread_t CounterThread;
#endif
typedef int32_t(*CounterRead)(const pc_virtual_counter*,uint64_t*);
typedef struct {
 uint32_t magic,version;void *host;CounterThread worker;
 pc_virtual_counter counter;CounterRead read;uint64_t frequency;
} Binding;
_Static_assert(sizeof(CPU)==448&&offsetof(CPU,rax)==0&&offsetof(CPU,rdx)==16&&offsetof(CPU,rip)==128,"selected whole CPU ABI");
_Static_assert(sizeof(Binding)==64&&_Alignof(Binding)==8&&sizeof(pc_virtual_counter)==24,"typed retained binding ABI");
#define MAGIC UINT32_C(0x50434342)
static _Thread_local const Binding *active_counter;
static int shape(const void*p)
{return p&&!((uintptr_t)p&7u)&&(uintptr_t)p<=UINTPTR_MAX-sizeof(Binding);}
static int32_t valid(const Binding*b)
{
 if(!shape(b)||b->magic!=MAGIC||b->version!=1||!b->host||!b->worker||b->read!=pc_virtual_counter_read||b->frequency!=UINT64_C(1000000000))return PC64_COUNTER_CONTEXT_V1;
 if(!pthread_equal(b->worker,pthread_self()))return PC64_COUNTER_WORKER_V1;
 return 0;
}
static int32_t get_active(const Binding**out)
{
 const Binding*b=active_counter;if(!b)return PC64_COUNTER_UNBOUND_V1;
 int32_t status=valid(b);if(status)return status;
 if(b->host!=pc64_active_host())return PC64_COUNTER_HOST_V1;
 *out=b;return 0;
}
static int output(const Binding*b,uint64_t*out)
{
 if(!out||((uintptr_t)out&7u)||(uintptr_t)out>UINTPTR_MAX-sizeof(*out))return 0;
 uintptr_t p=(uintptr_t)out,c=(uintptr_t)b;
 return p+sizeof(*out)<=c||c+sizeof(*b)<=p;
}
uint32_t pc64_counter_binding_size_v1(void){return(uint32_t)sizeof(Binding);}
int32_t pc64_counter_binding_init_v1(void*p,uint32_t bytes,void*host,const pc_virtual_counter*counter)
{
 if(pc64_dispatch_depth())return PC64_COUNTER_FRAME_V1;
 if(!shape(p)||bytes<sizeof(Binding)||!host||!counter)return PC64_PARAM;
 if(host!=pc64_active_host())return PC64_COUNTER_HOST_V1;
 for(size_t i=0;i<sizeof(Binding);i++)if(((const uint8_t*)p)[i])return PC64_COUNTER_CONTEXT_V1;
 uint64_t frequency;int32_t status=pc_virtual_counter_frequency(counter,&frequency);
 if(status||frequency!=UINT64_C(1000000000))return PC64_COUNTER_CONTEXT_V1;
 Binding candidate={MAGIC,1,host,pthread_self(),*counter,pc_virtual_counter_read,frequency};
 if(!candidate.worker)return PC64_COUNTER_WORKER_V1;
 *(Binding*)p=candidate;return 0;
}
int32_t pc64_counter_bind_v1(void*p)
{
 if(pc64_dispatch_depth())return PC64_COUNTER_FRAME_V1;
 const Binding*b=p;int32_t status=valid(b);if(status)return status;
 if(b->host!=pc64_active_host())return PC64_COUNTER_HOST_V1;
 if(active_counter&&active_counter!=b)return PC64_COUNTER_BUSY_V1;
 active_counter=b;return 0;
}
int32_t pc64_counter_unbind_after_stop_v1(void)
{if(pc64_dispatch_depth())return PC64_COUNTER_FRAME_V1;active_counter=0;return 0;}
#if defined(__clang__) || defined(__GNUC__)
__attribute__((noinline))
#endif
int32_t pc64_counter_read_active_v1(uint64_t*out)
{
 const Binding*b;int32_t status=get_active(&b);if(status)return status;
 if(!output(b,out))return PC64_PARAM;
 return b->read(&b->counter,out);
}
int32_t pc64_counter_frequency_active_v1(uint64_t*out)
{
 const Binding*b;int32_t status=get_active(&b);if(status)return status;
 if(!output(b,out))return PC64_PARAM;
 return pc_virtual_counter_frequency(&b->counter,out);
}
static _Noreturn void counter_fault(CPU*cpu,uint64_t sourceVA,int32_t status,uint64_t detail)
{cpu->rip=sourceVA;pc64_fail(status,sourceVA,detail);}
void pc64_required_counter(CPU*cpu,uint64_t sourceVA)
{
 if(!cpu)pc64_fail(PC64_PARAM,sourceVA,sizeof(CPU));
 uint64_t ticks;int32_t status=pc64_counter_read_active_v1(&ticks);
 if(status){
  if(status==PC64_COUNTER_UNBOUND_V1||status==PC64_COUNTER_HOST_V1||status==PC64_COUNTER_CONTEXT_V1||status==PC64_COUNTER_WORKER_V1)counter_fault(cpu,sourceVA,status,0);
  counter_fault(cpu,sourceVA,PC64_COUNTER_PROVIDER_V1,(uint32_t)status);
 }
 cpu->rax=(uint64_t)(uint32_t)ticks;
 cpu->rdx=(uint64_t)(uint32_t)(ticks>>32);
}
