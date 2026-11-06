#include "ngp_recursive_locks_v1.h"
#include "cookie_worker_tag.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct {pthread_mutex_t mutex;pthread_mutexattr_t attr;uint32_t depth;} Slot;
typedef struct {
 /* Immutable after release publication. All owners remain live to exit. */
 pthread_t owner;void *host;CPU *cpu;uint32_t tag,host_bytes,count,cancel_ack;
 NgpRecursiveLockDescriptorV1 descriptors[NGP_LOCK_SLOT_CAPACITY_V1];
 _Atomic uint32_t phase;
 /* Owner-only acknowledgement ledger, never read by foreign/observe. */
 uint32_t attempted,attr_init,type,mutex_init,attr_destroy_attempt,attr_destroy;
 uint32_t mutex_destroy,uncertain,peak_depth,stage;int32_t error;
 uint32_t secondary_stage;int32_t secondary_error;uint64_t last_identity;
 Slot slots[NGP_LOCK_SLOT_CAPACITY_V1];
#ifdef NGP_LOCK_TESTING
 uint64_t test_identity;uint32_t test_mask,calls[10];
#endif
} Service;

static _Atomic uint32_t attempt;
static _Atomic(Service*) canonical;
_Static_assert(sizeof(void*)==8&&sizeof(CPU)==448,"actual native64 CPU ABI");
_Static_assert(ATOMIC_INT_LOCK_FREE==2&&ATOMIC_POINTER_LOCK_FREE==2,"bounded publication");
_Static_assert(sizeof(NgpRecursiveLockDescriptorV1)==32&&_Alignof(NgpRecursiveLockDescriptorV1)==8,"descriptor ABI");
_Static_assert(sizeof(NgpRecursiveLocksReportV1)==88&&_Alignof(NgpRecursiveLocksReportV1)==8,"report ABI");

static int span(const void*p,size_t n,size_t align){
 uintptr_t x=(uintptr_t)p;return x&&n&&!(x%align)&&x<=UINTPTR_MAX-n;
}
static int overlaps(const void*a,size_t n,const void*b,size_t m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<y+m&&y<x+n;
}
static int report_valid(const Service*s,NgpRecursiveLocksReportV1*r){
 if(!span(r,sizeof*r,_Alignof(NgpRecursiveLocksReportV1)))return 0;
 if(s&&(overlaps(r,sizeof*r,s,sizeof*s)||overlaps(r,sizeof*r,s->host,s->host_bytes)||overlaps(r,sizeof*r,s->cpu,sizeof(CPU))))return 0;
 return r->version==1&&r->size==sizeof*r;
}
static int32_t emit(Service*s,NgpRecursiveLocksReportV1*r,int32_t status,int owner){
 NgpRecursiveLocksReportV1 out={0};out.version=1;out.size=sizeof out;out.status=status;
 if(s){
  out.phase=atomic_load_explicit(&s->phase,memory_order_acquire);
  if(owner){
   out.worker_observation=1;out.slot_count=s->count;out.cancel_disable_ack=s->cancel_ack;
   out.attr_init_ack_mask=s->attr_init;out.type_ack_mask=s->type;out.mutex_init_ack_mask=s->mutex_init;
   out.attr_destroy_ack_mask=s->attr_destroy;out.mutex_destroy_ack_mask=s->mutex_destroy;
   for(uint32_t i=0;i<s->count;i++)if(s->slots[i].depth)out.held_mask|=1u<<i;
   out.uncertain_mask=s->uncertain;out.max_held_depth=s->peak_depth;
   out.native_stage=s->stage;out.native_error=s->error;out.last_identity=s->last_identity;
   out.secondary_native_stage=s->secondary_stage;out.secondary_native_error=s->secondary_error;
  }
 }
 *r=out;return status;
}
/* A canonical NULL never authorizes reading proposed service storage. */
static int32_t actor(void*storage,CPU*cpu,NgpRecursiveLocksReportV1*r,int lane,Service**out){
 Service*s=atomic_load_explicit(&canonical,memory_order_acquire);
 if(!report_valid(s,r))return NGP_LOCK_ARGUMENT_V1;
 if(!s||storage!=s)return emit(NULL,r,NGP_LOCK_STATE_V1,0);
 uint32_t tag=0;
 if(cookie_worker_tag_read_v1(&tag)||tag!=s->tag)return emit(s,r,NGP_LOCK_OWNER_V1,0);
 if(!pthread_equal(pthread_self(),s->owner))return emit(s,r,NGP_LOCK_OWNER_V1,0);
 if(pc64_active_host()!=s->host)return emit(s,r,NGP_LOCK_HOST_V1,0);
 if(cpu!=s->cpu)return emit(s,r,NGP_LOCK_ARGUMENT_V1,0);
 uint32_t depth=pc64_dispatch_depth();
 if((lane==1&&depth!=0)||(lane==2&&depth==0))return emit(s,r,NGP_LOCK_FRAME_V1,1);
 *out=s;return 0;
}
static uint32_t wanted(const Service*s){return (1u<<s->count)-1u;}
static int complete(const Service*s){return NGP_LOCK_COMPLETE_V1(s->attr_init,s->type,s->mutex_init,s->attr_destroy,wanted(s));}
static int index_of(const Service*s,uint64_t identity){
 for(uint32_t i=0;i<s->count;i++)if(s->descriptors[i].identity==identity)return (int)i;
 return -1;
}
static void count_call(Service*s,uint32_t stage){
#ifdef NGP_LOCK_TESTING
 s->calls[stage]++;
#else
 (void)s;(void)stage;
#endif
}
static int lost_ack(Service*s,uint64_t identity,uint32_t stage){
#ifdef NGP_LOCK_TESTING
 uint32_t bit=1u<<stage;
 if(s->test_identity==identity&&(s->test_mask&bit)){s->test_mask&=~bit;return stage==NGP_LOCK_CLEANUP_V1?-9002:-9001;}
#else
 (void)s;(void)identity;(void)stage;
#endif
 return 0;
}
static int create_mutex(Service*s,Slot*x){count_call(s,NGP_LOCK_MUTEX_INIT_V1);return pthread_mutex_init(&x->mutex,&x->attr);}
static int destroy_mutex(Service*s,Slot*x){count_call(s,NGP_LOCK_MUTEX_DESTROY_V1);return pthread_mutex_destroy(&x->mutex);}
static int destroy_attribute(Service*s,Slot*x,uint32_t stage){count_call(s,stage);return pthread_mutexattr_destroy(&x->attr);}
static void first_failure(Service*s,uint32_t stage,int32_t error,uint32_t bit){
 if(!s->stage){s->stage=stage;s->error=error;}
 s->uncertain|=bit;atomic_store_explicit(&s->phase,NGP_LOCK_FAILED_V1,memory_order_release);
}
static int32_t failed(Service*s,NgpRecursiveLocksReportV1*r,uint32_t stage,int32_t error,uint32_t bit,int cleanup){
 first_failure(s,stage,error,bit);
 if(cleanup&&NGP_LOCK_ATTR_CLEANUP_V1(s->attr_init,s->attr_destroy_attempt,bit)){
  int index=index_of(s,s->last_identity); /* immutable identity selected by owner */
  if(index>=0){
   s->attr_destroy_attempt|=bit;
   int rc=destroy_attribute(s,&s->slots[index],NGP_LOCK_CLEANUP_V1);
   if(!rc)rc=lost_ack(s,s->last_identity,NGP_LOCK_CLEANUP_V1);
   if(rc){s->secondary_stage=NGP_LOCK_CLEANUP_V1;s->secondary_error=rc;}
   else s->attr_destroy|=bit;
  }
 }
 return emit(s,r,NGP_LOCK_RETAINED_V1,1);
}

uint32_t ngp_recursive_locks_size_v1(void){return (uint32_t)sizeof(Service);}
uint32_t ngp_recursive_locks_alignment_v1(void){return (uint32_t)_Alignof(Service);}

int32_t ngp_recursive_locks_init_v1(void*storage,uint32_t bytes,void*host,CPU*cpu,const NgpRecursiveLockDescriptorV1*d,uint32_t n,NgpRecursiveLocksReportV1*r){
 Service*s=storage;Service*known=atomic_load_explicit(&canonical,memory_order_acquire);
 if(known){if(!report_valid(known,r))return NGP_LOCK_ARGUMENT_V1;return emit(known,r,NGP_LOCK_STATE_V1,0);}
 uint32_t hb=pc64_host_size();
 if(!span(s,sizeof*s,_Alignof(Service))||bytes!=sizeof*s||!span(host,hb,8)||!span(cpu,sizeof(CPU),8)||!span(r,sizeof*r,_Alignof(NgpRecursiveLocksReportV1))||n==0||n>NGP_LOCK_SLOT_CAPACITY_V1||!span(d,(size_t)n*sizeof*d,_Alignof(NgpRecursiveLockDescriptorV1)))return NGP_LOCK_ARGUMENT_V1;
 if(overlaps(s,sizeof*s,host,hb)||overlaps(s,sizeof*s,cpu,sizeof(CPU))||overlaps(s,sizeof*s,d,(size_t)n*sizeof*d)||overlaps(s,sizeof*s,r,sizeof*r)||overlaps(host,hb,cpu,sizeof(CPU))||overlaps(host,hb,d,(size_t)n*sizeof*d)||overlaps(host,hb,r,sizeof*r)||overlaps(cpu,sizeof(CPU),d,(size_t)n*sizeof*d)||overlaps(cpu,sizeof(CPU),r,sizeof*r)||overlaps(d,(size_t)n*sizeof*d,r,sizeof*r))return NGP_LOCK_ARGUMENT_V1;
 if(!report_valid(NULL,r))return NGP_LOCK_ARGUMENT_V1;
 uint32_t tag=0;if(cookie_worker_tag_read_v1(&tag)||!tag)return emit(NULL,r,NGP_LOCK_OWNER_V1,0);
#if defined(__APPLE__)
 if(pthread_main_np())return emit(NULL,r,NGP_LOCK_OWNER_V1,0);
#else
 return emit(NULL,r,NGP_LOCK_PLATFORM_V1,0); /* Only genuine Darwin runtime is selected. */
#endif
 if(pc64_active_host()!=host)return emit(NULL,r,NGP_LOCK_HOST_V1,0);
 if(pc64_dispatch_depth()!=0)return emit(NULL,r,NGP_LOCK_FRAME_V1,0);
 for(uint32_t i=0;i<n;i++){
  if(d[i].version!=1||d[i].size!=sizeof*d||!d[i].identity||d[i].flags||d[i].reserved||d[i].reserved2)return NGP_LOCK_ARGUMENT_V1;
  for(uint32_t k=0;k<i;k++)if(d[k].identity==d[i].identity)return NGP_LOCK_ARGUMENT_V1;
 }
 for(size_t i=0;i<sizeof*s;i++)if(((const unsigned char*)s)[i])return NGP_LOCK_ARGUMENT_V1;
 uint32_t empty=0;
 if(!atomic_compare_exchange_strong_explicit(&attempt,&empty,1,memory_order_acq_rel,memory_order_acquire))return emit(NULL,r,NGP_LOCK_STATE_V1,0);
 /* The permanent claim has no exposed service pointer. Complete every
  * immutable field and initial cancellation/phase decision first. */
 s->owner=pthread_self();s->host=host;s->cpu=cpu;s->tag=tag;s->host_bytes=hb;s->count=n;
 for(uint32_t i=0;i<n;i++)s->descriptors[i]=d[i];
 atomic_init(&s->phase,NGP_LOCK_CONFIGURED_V1);
 int old=0;int rc=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&old);
 if(rc){s->stage=NGP_LOCK_CANCEL_V1;s->error=rc;atomic_store_explicit(&s->phase,NGP_LOCK_FAILED_V1,memory_order_relaxed);}
 else s->cancel_ack=1;
 atomic_store_explicit(&canonical,s,memory_order_release);
 return emit(s,r,rc?NGP_LOCK_PLATFORM_V1:0,1);
}

int32_t ngp_recursive_locks_construct_v1(void*storage,CPU*cpu,uint64_t identity,NgpRecursiveLocksReportV1*r){
 Service*s=NULL;int32_t q=actor(storage,cpu,r,2,&s);if(q)return q;
 uint32_t phase=atomic_load_explicit(&s->phase,memory_order_relaxed);
 if(phase!=NGP_LOCK_CONFIGURED_V1&&phase!=NGP_LOCK_CONSTRUCTING_V1)return emit(s,r,NGP_LOCK_STATE_V1,1);
 int index=index_of(s,identity);if(index<0)return emit(s,r,NGP_LOCK_IDENTITY_V1,1);
 uint32_t bit=1u<<(uint32_t)index;if(s->attempted&bit)return emit(s,r,NGP_LOCK_STATE_V1,1);
 s->attempted|=bit;s->last_identity=identity;atomic_store_explicit(&s->phase,NGP_LOCK_CONSTRUCTING_V1,memory_order_release);Slot*x=&s->slots[index];int rc;
 count_call(s,NGP_LOCK_ATTR_INIT_V1);rc=pthread_mutexattr_init(&x->attr);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_ATTR_INIT_V1);
 if(rc)return failed(s,r,NGP_LOCK_ATTR_INIT_V1,rc,bit,0);s->attr_init|=bit;
 count_call(s,NGP_LOCK_TYPE_V1);rc=pthread_mutexattr_settype(&x->attr,PTHREAD_MUTEX_RECURSIVE);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_TYPE_V1);
 if(rc)return failed(s,r,NGP_LOCK_TYPE_V1,rc,bit,1);s->type|=bit;
 rc=create_mutex(s,x);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_MUTEX_INIT_V1);
 if(rc)return failed(s,r,NGP_LOCK_MUTEX_INIT_V1,rc,bit,1);s->mutex_init|=bit;
 s->attr_destroy_attempt|=bit;rc=destroy_attribute(s,x,NGP_LOCK_ATTR_DESTROY_V1);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_ATTR_DESTROY_V1);
 if(rc)return failed(s,r,NGP_LOCK_ATTR_DESTROY_V1,rc,bit,0);s->attr_destroy|=bit;
 if(complete(s))atomic_store_explicit(&s->phase,NGP_LOCK_LIVE_V1,memory_order_release);
 return emit(s,r,0,1);
}

int32_t ngp_recursive_locks_enter_v1(void*storage,CPU*cpu,uint64_t identity,NgpRecursiveLocksReportV1*r){
 Service*s=NULL;int32_t q=actor(storage,cpu,r,2,&s);if(q)return q;
 if(atomic_load_explicit(&s->phase,memory_order_relaxed)!=NGP_LOCK_LIVE_V1||!complete(s))return emit(s,r,NGP_LOCK_STATE_V1,1);
 int index=index_of(s,identity);if(index<0)return emit(s,r,NGP_LOCK_IDENTITY_V1,1);Slot*x=&s->slots[index];uint32_t bit=1u<<(uint32_t)index;
 if(!NGP_LOCK_ENTER_ALLOWED_V1(x->depth))return emit(s,r,NGP_LOCK_LIMIT_V1,1);
 s->last_identity=identity;count_call(s,NGP_LOCK_ENTER_V1);int rc=pthread_mutex_lock(&x->mutex);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_ENTER_V1);
 if(rc)return failed(s,r,NGP_LOCK_ENTER_V1,rc,bit,0);
 x->depth++;if(x->depth>s->peak_depth)s->peak_depth=x->depth;
 return emit(s,r,0,1);
}

int32_t ngp_recursive_locks_leave_v1(void*storage,CPU*cpu,uint64_t identity,NgpRecursiveLocksReportV1*r){
 Service*s=NULL;int32_t q=actor(storage,cpu,r,2,&s);if(q)return q;
 if(atomic_load_explicit(&s->phase,memory_order_relaxed)!=NGP_LOCK_LIVE_V1||!complete(s))return emit(s,r,NGP_LOCK_STATE_V1,1);
 int index=index_of(s,identity);if(index<0)return emit(s,r,NGP_LOCK_IDENTITY_V1,1);Slot*x=&s->slots[index];uint32_t bit=1u<<(uint32_t)index;
 if(!NGP_LOCK_LEAVE_ALLOWED_V1(x->depth))return emit(s,r,NGP_LOCK_STATE_V1,1);
 s->last_identity=identity;count_call(s,NGP_LOCK_LEAVE_V1);int rc=pthread_mutex_unlock(&x->mutex);
 if(!rc)rc=lost_ack(s,identity,NGP_LOCK_LEAVE_V1);
 if(rc)return failed(s,r,NGP_LOCK_LEAVE_V1,rc,bit,0);x->depth--;
 return emit(s,r,0,1);
}

int32_t ngp_recursive_locks_retire_v1(void*storage,CPU*cpu,NgpRecursiveLocksReportV1*r){
 Service*s=NULL;int32_t q=actor(storage,cpu,r,1,&s);if(q)return q;
 uint32_t phase=atomic_load_explicit(&s->phase,memory_order_relaxed);
 if(phase==NGP_LOCK_CLOSED_V1)return emit(s,r,0,1);
 if(phase!=NGP_LOCK_LIVE_V1||!complete(s))return emit(s,r,NGP_LOCK_STATE_V1,1);
 for(uint32_t i=0;i<s->count;i++)if(s->slots[i].depth)return emit(s,r,NGP_LOCK_BUSY_V1,1);
 atomic_store_explicit(&s->phase,NGP_LOCK_RETIRING_V1,memory_order_release);
 for(uint32_t i=0;i<s->count;i++){
  uint32_t bit=1u<<i;s->last_identity=s->descriptors[i].identity;int rc=destroy_mutex(s,&s->slots[i]);
  if(!rc)rc=lost_ack(s,s->last_identity,NGP_LOCK_MUTEX_DESTROY_V1);
  if(rc)return failed(s,r,NGP_LOCK_MUTEX_DESTROY_V1,rc,bit,0);s->mutex_destroy|=bit;
 }
 atomic_store_explicit(&s->phase,NGP_LOCK_CLOSED_V1,memory_order_release);return emit(s,r,0,1);
}

int32_t ngp_recursive_locks_observe_v1(void*storage,NgpRecursiveLocksReportV1*r){
 Service*s=atomic_load_explicit(&canonical,memory_order_acquire);
 if(!report_valid(s,r))return NGP_LOCK_ARGUMENT_V1;
 if(!s||storage!=s)return emit(NULL,r,NGP_LOCK_STATE_V1,0);
 return emit(s,r,0,0);
}
#ifdef NGP_LOCK_TESTING
int32_t ngp_recursive_locks_test_fault_v1(void*storage,CPU*cpu,uint64_t identity,uint32_t mask){
 NgpRecursiveLocksReportV1 r={0};r.version=1;r.size=sizeof r;Service*s=NULL;int32_t q=actor(storage,cpu,&r,1,&s);if(q)return q;
 uint32_t phase=atomic_load_explicit(&s->phase,memory_order_relaxed);
 if(phase!=NGP_LOCK_CONFIGURED_V1&&phase!=NGP_LOCK_CONSTRUCTING_V1&&phase!=NGP_LOCK_LIVE_V1)return NGP_LOCK_STATE_V1;
 if(index_of(s,identity)<0||!mask||(mask&~1020u)||s->test_mask)return NGP_LOCK_ARGUMENT_V1;
 s->test_identity=identity;s->test_mask=mask;return 0;
}
int32_t ngp_recursive_locks_test_stats_v1(void*storage,CPU*cpu,NgpRecursiveLocksTestStatsV1*out){
 NgpRecursiveLocksReportV1 r={0};r.version=1;r.size=sizeof r;Service*s=NULL;int32_t q=actor(storage,cpu,&r,0,&s);if(q)return q;
 if(!span(out,sizeof*out,_Alignof(NgpRecursiveLocksTestStatsV1))||overlaps(out,sizeof*out,s,sizeof*s)||overlaps(out,sizeof*out,s->host,s->host_bytes)||overlaps(out,sizeof*out,s->cpu,sizeof(CPU))||out->version!=1||out->size!=sizeof*out)return NGP_LOCK_ARGUMENT_V1;
 for(uint32_t i=0;i<10;i++)out->calls[i]=s->calls[i];return 0;
}
#endif
