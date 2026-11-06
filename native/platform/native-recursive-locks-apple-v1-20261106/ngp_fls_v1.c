#include "ngp_fls_v1.h"
#include "ngp_worker_tag_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
typedef struct Service Service;
typedef struct {Service *service;void *value;uint32_t ordinal;} Box;
struct Service {
 pthread_t owner;void *host;uint32_t worker_tag;NgpFlsCallbackV1 callback;
 _Atomic uint32_t published_phase,owner_exited;
 NgpFlsPolicyV1 policy;pthread_key_t key;uint32_t token,boxes_used,key_created;
 uint32_t callback_required,callback_attempted;int32_t native_error;
 Box *current;Box boxes[NGP_FLS_BOX_CAPACITY];
#ifdef NGP_FLS_TESTING
 uint32_t test_fault;
#endif
};
static _Atomic(Service*) canonical;
static _Atomic uint32_t process_ready;
static pthread_t process_main;
static uint32_t last_key; /* Sole canonical owner, no reset/wrap/reuse. */
_Static_assert(sizeof(void*)==8&&ATOMIC_INT_LOCK_FREE==2&&ATOMIC_POINTER_LOCK_FREE==2,"bounded native64 lock-free publication required");
_Static_assert(sizeof(NgpFlsReportV1)==60,"version1 report ABI");
static int span(const void*p,size_t n,size_t align){uintptr_t a=(uintptr_t)p;return a&&!(a%(uintptr_t)align)&&n&&a<=UINTPTR_MAX-n;}
static int overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<y+m&&y<x+n;}
static int report_valid(Service*s,NgpFlsReportV1*r){return span(r,sizeof*r,_Alignof(NgpFlsReportV1))&&(!s||!overlap(r,sizeof*r,s,sizeof*s))&&r->version==1&&r->size==sizeof*r;}
static int output_valid(Service*s,const void*p,size_t n,size_t align,NgpFlsReportV1*r){return span(p,n,align)&&!overlap(p,n,s,sizeof*s)&&!overlap(p,n,s->host,ngp_fls_environment_host_bytes_v1())&&!overlap(p,n,r,sizeof*r);}
static int32_t emit(Service*s,NgpFlsReportV1*r,int32_t status,int owner){
 NgpFlsReportV1 n={0};n.version=1;n.size=sizeof n;n.status=status;
 if(s){n.phase=atomic_load_explicit(&s->published_phase,memory_order_acquire);n.owner_exit_observed=atomic_load_explicit(&s->owner_exited,memory_order_acquire);if(n.owner_exit_observed&&n.phase!=NGP_FLS_CLOSED)n.phase=NGP_FLS_OWNER_EXITED;
  if(owner){n.worker_observation=1;n.token=s->token;n.boxes_used=s->boxes_used;n.key_created=s->key_created;n.value_clear_ack=(s->policy.acknowledgements&1)!=0;n.callback_required=s->callback_required;n.callback_attempted=s->callback_attempted;n.callback_ack=(s->policy.acknowledgements&2)!=0;n.key_delete_ack=(s->policy.acknowledgements&4)!=0;n.native_error=s->native_error;}}
 *r=n;return status;
}
static void publish(Service*s){atomic_store_explicit(&s->published_phase,s->policy.phase,memory_order_release);}
static int32_t fail(Service*s,NgpFlsReportV1*r,int32_t status,int32_t native){s->native_error=native;(void)ngp_fls_step_v1(&s->policy,NGP_FLS_FAIL);publish(s);return emit(s,r,status,1);}
static int32_t owner_check(Service*s,NgpFlsReportV1*r){
 Service*known=atomic_load_explicit(&canonical,memory_order_acquire);
 if(known&&span(r,sizeof*r,_Alignof(NgpFlsReportV1))&&overlap(r,sizeof*r,known->host,ngp_fls_environment_host_bytes_v1()))return NGP_FLS_ARGUMENT;
 if(!report_valid(known,r))return NGP_FLS_ARGUMENT;
 if(!s||s!=known)return emit(NULL,r,NGP_FLS_ARGUMENT,0);
 uint32_t tag=0;if(ngp_worker_tag_read_v1(&tag)||tag!=s->worker_tag)return emit(s,r,NGP_FLS_OWNER,0);
 if(!pthread_equal(pthread_self(),s->owner))return emit(s,r,NGP_FLS_OWNER,0);
 if(ngp_fls_environment_active_v1()!=s->host)return emit(s,r,NGP_FLS_HOST,0);
 if(atomic_load_explicit(&s->owner_exited,memory_order_acquire))return emit(s,r,NGP_FLS_RETAINED,0);
 return 0;
}
static int32_t live(Service*s,uint32_t token,NgpFlsReportV1*r){int32_t q=owner_check(s,r);if(q)return q;if(s->policy.phase!=NGP_FLS_LIVE||token!=s->token)return emit(s,r,NGP_FLS_STATE,1);return 0;}
/* OS callback: immutable retained box->service, then one proven-lockfree store.
 * No guest CPU, callback, free, allocation, source lock, key or ledger access. */
static void owner_exit(void*p){Box*b=p;if(b)atomic_store_explicit(&b->service->owner_exited,1,memory_order_release);}
static int injected(Service*s,uint32_t stage){
#ifdef NGP_FLS_TESTING
 if(s->test_fault==stage){s->test_fault=0;return -9001;}
#else
 (void)s;(void)stage;
#endif
 return 0;
}
int32_t ngp_fls_prepare_process_v1(void){
#if defined(__APPLE__)
 if(!pthread_main_np())return NGP_FLS_OWNER;
#endif
 if(ngp_fls_environment_depth_v1()!=0)return NGP_FLS_STATE;
 uint32_t expected=0;if(!atomic_compare_exchange_strong_explicit(&process_ready,&expected,1,memory_order_acq_rel,memory_order_acquire))return NGP_FLS_STATE;
 process_main=pthread_self();atomic_store_explicit(&process_ready,2,memory_order_release);return 0;
}
uint32_t ngp_fls_size_v1(void){return (uint32_t)sizeof(Service);}
uint32_t ngp_fls_alignment_v1(void){return (uint32_t)_Alignof(Service);}
int32_t ngp_fls_init_v1(void*storage,uint32_t bytes,void*host,const NgpFlsCallbackV1*cb,NgpFlsReportV1*r){
 Service*s=storage;Service*known=atomic_load_explicit(&canonical,memory_order_acquire);
 if(known&&span(r,sizeof*r,_Alignof(NgpFlsReportV1))&&overlap(r,sizeof*r,known,sizeof*known))return NGP_FLS_ARGUMENT;
 if(!span(s,sizeof*s,_Alignof(Service))||bytes!=sizeof*s||!report_valid(s,r)||!span(host,ngp_fls_environment_host_bytes_v1(),8)||overlap(s,sizeof*s,host,ngp_fls_environment_host_bytes_v1())||overlap(r,sizeof*r,host,ngp_fls_environment_host_bytes_v1())||!span(cb,sizeof*cb,_Alignof(NgpFlsCallbackV1))||overlap(cb,sizeof*cb,s,sizeof*s)||overlap(cb,sizeof*cb,host,ngp_fls_environment_host_bytes_v1())||overlap(cb,sizeof*cb,r,sizeof*r)||!cb->function)return NGP_FLS_ARGUMENT;
 if(known)return emit(known,r,NGP_FLS_STATE,0);
 if(atomic_load_explicit(&process_ready,memory_order_acquire)!=2)return emit(NULL,r,NGP_FLS_STATE,0);
 if(pthread_equal(pthread_self(),process_main))return emit(NULL,r,NGP_FLS_OWNER,0);
 if(ngp_fls_environment_depth_v1()!=0)return emit(NULL,r,NGP_FLS_STATE,0);
 if(ngp_fls_environment_active_v1()!=host)return emit(NULL,r,NGP_FLS_HOST,0);
 for(size_t i=0;i<sizeof*s;i++)if(((unsigned char*)s)[i])return NGP_FLS_ARGUMENT;
 uint32_t tag=0;int32_t status=ngp_worker_tag_warm_v1(&tag);if(status||!tag)return emit(NULL,r,NGP_FLS_OWNER,0);
 s->owner=pthread_self();s->host=host;s->worker_tag=tag;s->callback=*cb;s->policy.phase=NGP_FLS_INIT;s->token=UINT32_MAX;atomic_init(&s->published_phase,NGP_FLS_INIT);atomic_init(&s->owner_exited,0);
 Service*empty=NULL;if(!atomic_compare_exchange_strong_explicit(&canonical,&empty,s,memory_order_acq_rel,memory_order_acquire))return emit(empty,r,NGP_FLS_STATE,0);
 int old=0;int rc=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&old);if(rc)return fail(s,r,NGP_FLS_PLATFORM,rc);
 return emit(s,r,0,1);
}
int32_t ngp_fls_allocate_v1(void*storage,uint32_t*out,NgpFlsReportV1*r){Service*s=storage;int32_t q=owner_check(s,r);if(q)return q;if(!output_valid(s,out,sizeof*out,_Alignof(uint32_t),r))return NGP_FLS_ARGUMENT;if(*out!=UINT32_MAX||s->policy.phase!=NGP_FLS_INIT)return emit(s,r,NGP_FLS_STATE,1);
 uint32_t next=UINT32_MAX;q=ngp_fls_key_next_v1(last_key,&next);if(q)return fail(s,r,NGP_FLS_LIMIT,0);last_key=next;
 int rc=pthread_key_create(&s->key,owner_exit);if(rc)return fail(s,r,NGP_FLS_PLATFORM,rc);s->key_created=1;
 if((rc=injected(s,1)))return fail(s,r,NGP_FLS_RETAINED,rc);
 s->token=next;q=ngp_fls_step_v1(&s->policy,NGP_FLS_ALLOCATED);if(q)return fail(s,r,q,0);publish(s);*out=next;return emit(s,r,0,1);
}
int32_t ngp_fls_get_v1(void*storage,uint32_t token,void**out,NgpFlsReportV1*r){Service*s=storage;int32_t q=live(s,token,r);if(q)return q;if(!output_valid(s,out,sizeof*out,_Alignof(void*),r))return NGP_FLS_ARGUMENT;void*actual=pthread_getspecific(s->key);if(actual!=s->current)return fail(s,r,NGP_FLS_RETAINED,0);*out=s->current?s->current->value:NULL;return emit(s,r,0,1);}
int32_t ngp_fls_set_v1(void*storage,uint32_t token,void*value,NgpFlsReportV1*r){Service*s=storage;int32_t q=live(s,token,r);if(q)return q;
 if(value&&(!span(value,1,1)||overlap(value,1,s,sizeof*s)||overlap(value,1,s->host,ngp_fls_environment_host_bytes_v1())||overlap(value,1,r,sizeof*r)))return NGP_FLS_ARGUMENT;
 Box*b=NULL;if(value){if(s->boxes_used==NGP_FLS_BOX_CAPACITY)return emit(s,r,NGP_FLS_LIMIT,1);b=&s->boxes[s->boxes_used];b->service=s;b->value=value;b->ordinal=++s->boxes_used;}
 int rc=pthread_setspecific(s->key,b);if(rc)return fail(s,r,NGP_FLS_RETAINED,rc);if((rc=injected(s,2)))return fail(s,r,NGP_FLS_RETAINED,rc);s->current=b;return emit(s,r,0,1);
}
int32_t ngp_fls_retire_v1(void*storage,uint32_t token,NgpFlsReportV1*r){Service*s=storage;int32_t q=owner_check(s,r);if(q)return q;if(token!=s->token)return emit(s,r,NGP_FLS_STATE,1);if(s->policy.phase==NGP_FLS_CLOSED)return emit(s,r,0,1);if(s->policy.phase!=NGP_FLS_LIVE||ngp_fls_environment_depth_v1()!=0)return emit(s,r,NGP_FLS_STATE,1);
 if(pthread_getspecific(s->key)!=s->current)return fail(s,r,NGP_FLS_RETAINED,0);
 q=ngp_fls_step_v1(&s->policy,NGP_FLS_BEGIN_RETIRE);if(q)return fail(s,r,q,0);publish(s);Box*b=s->current;s->callback_required=b!=NULL;
 int rc=pthread_setspecific(s->key,NULL);if(rc)return fail(s,r,NGP_FLS_RETAINED,rc);if((rc=injected(s,3)))return fail(s,r,NGP_FLS_RETAINED,rc);
 q=ngp_fls_step_v1(&s->policy,NGP_FLS_CLEAR_ACK);if(q)return fail(s,r,q,0);
 if(b){s->callback_attempted=1;rc=s->callback.function(s->callback.context,b->value);if(rc)return fail(s,r,NGP_FLS_CALLBACK,rc);}
 q=ngp_fls_step_v1(&s->policy,NGP_FLS_CALLBACK_ACK);if(q)return fail(s,r,q,0);
 rc=pthread_key_delete(s->key);if(rc)return fail(s,r,NGP_FLS_RETAINED,rc);if((rc=injected(s,4)))return fail(s,r,NGP_FLS_RETAINED,rc);
 q=ngp_fls_step_v1(&s->policy,NGP_FLS_DELETE_ACK);if(q)return fail(s,r,q,0);s->current=NULL;publish(s);return emit(s,r,0,1);
}
int32_t ngp_fls_observe_v1(void*storage,NgpFlsReportV1*r){Service*s=atomic_load_explicit(&canonical,memory_order_acquire);if(s&&span(r,sizeof*r,_Alignof(NgpFlsReportV1))&&overlap(r,sizeof*r,s->host,ngp_fls_environment_host_bytes_v1()))return NGP_FLS_ARGUMENT;if(!report_valid(s,r))return NGP_FLS_ARGUMENT;if(!s||storage!=s)return emit(NULL,r,NGP_FLS_ARGUMENT,0);return emit(s,r,0,0);}
#ifdef NGP_FLS_TESTING
int32_t ngp_fls_test_fault_v1(void*storage,uint32_t stage){Service*s=storage;NgpFlsReportV1 r={0};r.version=1;r.size=sizeof r;int32_t q=owner_check(s,&r);if(q)return q;if(stage<1||stage>4||ngp_fls_environment_depth_v1()!=0)return NGP_FLS_ARGUMENT;s->test_fault=stage;return 0;}
int32_t ngp_fls_test_late_exit_v1(void*storage){Service*s=storage;NgpFlsReportV1 r={0};r.version=1;r.size=sizeof r;int32_t q=owner_check(s,&r);if(q)return q;if(!s->boxes_used)return NGP_FLS_STATE;owner_exit(&s->boxes[0]);return 0;}
#endif
