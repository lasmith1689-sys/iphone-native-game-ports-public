/* Own native protocol adapter. Link the real unchanged voice backend in product
 * builds. Controlled tests deliberately link a separately labelled test backend. */
#include "ngp_voice_tokens_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint64_t native;uint32_t token;} Entry;
struct NgpVoiceTokensV1 {
 _Atomic uint32_t phase;
 uint32_t kind,capacity,last_serial,reserved,loans;
 pthread_t owner;NgpVoiceServiceV1 *backend;
 int32_t last_status;NgpVoiceReportV1 last_voice;Entry entries[32];
};
static _Atomic(NgpVoiceTokensV1*) canonical[3];
static int report_valid(const NgpVoiceTokensReportV1*r){return r&&r->version==1&&r->size==sizeof(*r);}
static NgpVoiceReportV1 voice_report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
/* Compare against retained canonical pointers before any context dereference. */
static uint32_t known(const NgpVoiceTokensV1*s){if(!s)return 0;for(uint32_t i=0;i<3;++i)if(atomic_load_explicit(&canonical[i],memory_order_acquire)==s)return i+1;return 0;}
static int emit(NgpVoiceTokensV1*s,NgpVoiceTokensReportV1*r,int code,int worker){
 memset(r,0,sizeof *r);r->version=1;r->size=sizeof *r;r->status=(uint32_t)code;r->voice=voice_report();
 uint32_t kind=known(s);if(kind){r->kind=kind;r->capacity=kind==3?13:32;r->phase=atomic_load_explicit(&s->phase,memory_order_acquire);}
 if(worker){r->worker_observation=1;r->reserved_tracked=s->reserved;r->loans_tracked=s->loans;r->backend_status=s->last_status;r->voice=s->last_voice;}
 return code;
}
/* STARTING/RESERVED have no published owner/backend yet. */
static int owner(NgpVoiceTokensV1*s,NgpVoiceTokensReportV1*r,uint32_t*p){
 if(!report_valid(r))return NGP_TOKENS_ARGUMENT;
 if(!known(s))return emit(NULL,r,NGP_TOKENS_ARGUMENT,0);
 *p=atomic_load_explicit(&s->phase,memory_order_acquire);
 if(*p<NGP_TOKENS_PREPARING)return emit(s,r,NGP_TOKENS_STATE,0);
 if(!pthread_equal(s->owner,pthread_self()))return emit(s,r,NGP_TOKENS_OWNER,0);
 return 0;
}
static int ready(NgpVoiceTokensV1*s,NgpVoiceTokensReportV1*r){uint32_t p=0;int code=owner(s,r,&p);if(code)return code;return p==NGP_TOKENS_READY?0:emit(s,r,p==NGP_TOKENS_FAILED?NGP_TOKENS_RETAINED:NGP_TOKENS_STATE,1);}
static int failed(NgpVoiceTokensV1*s,NgpVoiceTokensReportV1*r){atomic_store_explicit(&s->phase,NGP_TOKENS_FAILED,memory_order_release);return emit(s,r,NGP_TOKENS_BACKEND,1);}
static Entry*find(NgpVoiceTokensV1*s,uint32_t token){if(!token||(token&3u)!=s->kind)return NULL;for(uint32_t i=0;i<s->capacity;++i)if(s->entries[i].token==token)return&s->entries[i];return NULL;}
static int distinct(NgpVoiceTokensV1*s,uint64_t h){if(!h)return 0;for(uint32_t i=0;i<s->capacity;++i)if(s->entries[i].native==h)return 0;return 1;}
static int snapshot(NgpVoiceTokensV1*s,Entry*e,NgpVoiceTokensReportV1*r){
 NgpVoiceReportV1 v=voice_report();int code=ngp_voice_service_v1_snapshot(s->backend,e->native,&v);s->last_status=code;s->last_voice=v;
 if(code||v.handle!=e->native||v.kind!=s->kind)return failed(s,r);
 return 0;
}
int ngp_voice_tokens_v1_reserve(uint32_t kind,NgpVoiceTokensV1**out,NgpVoiceTokensReportV1*r){
 if(!report_valid(r))return NGP_TOKENS_ARGUMENT;
 if(kind<1||kind>3||!out||*out)return emit(NULL,r,NGP_TOKENS_ARGUMENT,0);
 if(atomic_load_explicit(&canonical[kind-1],memory_order_acquire))return emit(NULL,r,NGP_TOKENS_STATE,0);
 NgpVoiceTokensV1*s=calloc(1,sizeof *s);if(!s)return emit(NULL,r,NGP_TOKENS_ALLOC,0);
 s->kind=kind;s->capacity=kind==3?13:32;s->last_voice=voice_report();atomic_init(&s->phase,NGP_TOKENS_RESERVED);
 NgpVoiceTokensV1*empty=NULL;if(!atomic_compare_exchange_strong_explicit(&canonical[kind-1],&empty,s,memory_order_acq_rel,memory_order_acquire)){free(s);return emit(NULL,r,NGP_TOKENS_STATE,0);}
 *out=s;return emit(s,r,0,0);
}
int ngp_voice_tokens_v1_prepare(NgpVoiceTokensV1*s,NgpVoiceServiceV1*backend,NgpVoiceTokensReportV1*r){
 if(!report_valid(r))return NGP_TOKENS_ARGUMENT;
 if(!known(s)||!backend)return emit(NULL,r,NGP_TOKENS_ARGUMENT,0);
 uint32_t expected=NGP_TOKENS_RESERVED;
 if(!atomic_compare_exchange_strong_explicit(&s->phase,&expected,NGP_TOKENS_STARTING,memory_order_acq_rel,memory_order_acquire)){
  uint32_t p=0;int code=owner(s,r,&p);return code?code:emit(s,r,NGP_TOKENS_STATE,1);
 }
 NgpVoiceReportV1 v=voice_report();uint64_t h=0;int code=ngp_voice_service_v1_acquire(backend,s->kind,&h,&v);
 /* The frozen backend's OWNER refusal precedes any effect and output write.
  * Other failures may be partial; never infer rollback from a zero output. */
 if(code==NGP_VOICE_OWNER_V1&&!h){atomic_store_explicit(&s->phase,NGP_TOKENS_RESERVED,memory_order_release);return emit(s,r,NGP_TOKENS_OWNER,0);}
 s->owner=pthread_self();s->backend=backend;s->last_status=code;s->last_voice=v;
 atomic_store_explicit(&s->phase,NGP_TOKENS_PREPARING,memory_order_release);
 if(code||!h||v.handle!=h||v.kind!=s->kind){s->entries[0].native=h;return failed(s,r);}
 s->entries[0].native=h;s->reserved=1;
 for(uint32_t i=1;i<s->capacity;++i){
  h=0;v=voice_report();code=ngp_voice_service_v1_acquire(backend,s->kind,&h,&v);s->last_status=code;s->last_voice=v;
  if(code||!distinct(s,h)||v.handle!=h||v.kind!=s->kind){s->entries[i].native=h;return failed(s,r);}
  s->entries[i].native=h;++s->reserved;
 }
 atomic_store_explicit(&s->phase,NGP_TOKENS_READY,memory_order_release);return emit(s,r,0,1);
}
int ngp_voice_tokens_v1_assign(NgpVoiceTokensV1*s,uint32_t*out,NgpVoiceTokensReportV1*r){
 int code=ready(s,r);if(code)return code;if(!out||*out)return emit(s,r,NGP_TOKENS_ARGUMENT,1);
 if(s->last_serial==UINT32_MAX>>2)return emit(s,r,NGP_TOKENS_LIMIT,1);
 Entry*e=NULL;for(uint32_t i=0;i<s->capacity;++i)if(!s->entries[i].token){e=&s->entries[i];break;}
 if(!e)return emit(s,r,NGP_TOKENS_LIMIT,1);if((code=snapshot(s,e,r)))return code;
 uint32_t token=((s->last_serial+1)<<2)|s->kind;++s->last_serial;e->token=token;++s->loans;*out=token;return emit(s,r,0,1);
}
int ngp_voice_tokens_v1_resolve(NgpVoiceTokensV1*s,uint32_t token,uint64_t*out,NgpVoiceTokensReportV1*r){
 int code=ready(s,r);if(code)return code;if(!out||*out)return emit(s,r,NGP_TOKENS_ARGUMENT,1);
 Entry*e=find(s,token);if(!e)return emit(s,r,NGP_TOKENS_TOKEN,1);if((code=snapshot(s,e,r)))return code;
 *out=e->native;return emit(s,r,0,1);
}
int ngp_voice_tokens_v1_release(NgpVoiceTokensV1*s,uint32_t token,NgpVoiceTokensReportV1*r){
 int code=ready(s,r);if(code)return code;Entry*e=find(s,token);if(!e)return emit(s,r,NGP_TOKENS_TOKEN,1);
 NgpVoiceReportV1 v=voice_report();code=ngp_voice_service_v1_release(s->backend,e->native,&v);s->last_status=code;s->last_voice=v;if(code)return failed(s,r);
 e->native=0;e->token=0;--s->reserved;--s->loans;
 uint64_t h=0;v=voice_report();code=ngp_voice_service_v1_acquire(s->backend,s->kind,&h,&v);s->last_status=code;s->last_voice=v;
 if(code||!distinct(s,h)||v.handle!=h||v.kind!=s->kind){e->native=h;return failed(s,r);}
 e->native=h;++s->reserved;return emit(s,r,0,1);
}
int ngp_voice_tokens_v1_close(NgpVoiceTokensV1*s,NgpVoiceTokensReportV1*r){
 uint32_t phase=0;int code=owner(s,r,&phase);if(code)return code;
 if(phase==NGP_TOKENS_CLOSED)return emit(s,r,0,1);
 if(phase!=NGP_TOKENS_READY)return emit(s,r,phase==NGP_TOKENS_FAILED?NGP_TOKENS_RETAINED:NGP_TOKENS_STATE,1);
 for(uint32_t i=0;i<s->capacity;++i){Entry*e=&s->entries[i];NgpVoiceReportV1 v=voice_report();code=ngp_voice_service_v1_release(s->backend,e->native,&v);s->last_status=code;s->last_voice=v;if(code)return failed(s,r);
  e->native=0;--s->reserved;if(e->token){e->token=0;--s->loans;}
 }
 atomic_store_explicit(&s->phase,NGP_TOKENS_CLOSED,memory_order_release);return emit(s,r,0,1);
}
