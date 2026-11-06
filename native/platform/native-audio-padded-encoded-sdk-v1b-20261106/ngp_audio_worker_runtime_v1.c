/* Own orchestration. Production links the actual broker bridge and real backends. */
#include "ngp_audio_worker_runtime_v1.h"
#include "ngp_audio_stop_os_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
_Static_assert(sizeof(NgpAudioWorkerStopReportV1)==64,"stop report ABI64");
_Static_assert(offsetof(NgpAudioWorkerStopReportV1,intent_epoch)==56,"stop report epoch offset56");
/* Apple supplies this declaration; controlled Windows builds supply the
 * separately labelled test definition, never a product main-thread substitute. */
extern int pthread_main_np(void);
struct NgpAudioWorkerRuntimeV1 {
 pthread_t owner;void*broker;NgpDeviceMixerFactsV1 facts;
 NgpAudioWorkerContextsV1 contexts;_Atomic uint32_t phase;
 uint32_t ordinary_reserved,positional_reserved,stream_reserved;
 uint32_t voice_prepared,gate_enabled,stop_request_returned;
 int32_t backend_status,stop_request_status;
 uint32_t stop_consumed,stop_request_started,close_attempted,close_ack;
 uint32_t first_worker_status,broker_outcome;
 uint32_t wait_reason,wait_detail_status,wait_attempts;
 int32_t wait_native_error;
 uint64_t wait_start,wait_deadline,wait_last;
};
static _Atomic(NgpAudioWorkerRuntimeV1*) canonical;
static int valid(const NgpAudioWorkerReportV1*r){return r&&r->version==1&&r->size==sizeof *r;}
static int known(const NgpAudioWorkerRuntimeV1*s){return s&&atomic_load_explicit(&canonical,memory_order_acquire)==s;}
static int emit(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerReportV1*r,int code,int worker){
 NgpAudioWorkerReportV1 v={0};v.version=1;v.size=sizeof v;v.status=(uint32_t)code;
 if(s)v.phase=atomic_load_explicit(&s->phase,memory_order_acquire);
 if(worker){v.worker_observation=1;v.ordinary_reserved=s->ordinary_reserved;v.positional_reserved=s->positional_reserved;v.stream_reserved=s->stream_reserved;
  v.voice_prepared=s->voice_prepared;v.gate_enabled=s->gate_enabled;v.stop_request_returned=s->stop_request_returned;
  v.backend_status=s->backend_status;v.stop_request_status=s->stop_request_status;v.intent_epoch=s->facts.intent_epoch;}
 *r=v;return code;
}
static NgpVoiceReportV1 voice_report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 token_report(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpPositionalReportV1 positional_report(void){NgpPositionalReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpDeviceMixerFactsV1 facts_report(void){NgpDeviceMixerFactsV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int facts_valid(const NgpDeviceMixerFactsV1*f){
 return f->version==1&&f->size==sizeof *f&&f->phase==NGP_DEVICE_BROKER_ACTIVE_V1&&f->voice_service&&
  (f->sample_rate==44100||f->sample_rate==48000)&&(f->channels==1||f->channels==2)&&
  f->maximum_frames&&f->maximum_frames<=NGP_VOICE_MAX_RENDER_FRAMES_V1&&(f->intent_epoch&3u)==3u;
}
static void first_failure(NgpAudioWorkerRuntimeV1*s,uint32_t own,int backend){
 if(!s->first_worker_status){s->first_worker_status=own;s->backend_status=backend;}
 atomic_store_explicit(&s->phase,NGP_AUDIO_WORKER_FAILED_RETAINED,memory_order_release);
}
static void request_once(NgpAudioWorkerRuntimeV1*s){
 s->stop_consumed=1;
 if(s->stop_request_started)return;
 s->stop_request_started=1; /* Claim before calling the real bridge. */
 s->stop_request_status=ngp_audio_worker_broker_request_stop_v1(s->broker);s->stop_request_returned=1;
 if(s->stop_request_status&&s->stop_request_status!=NGP_DEVICE_BROKER_PENDING_V1)first_failure(s,NGP_AUDIO_WORKER_BROKER,s->stop_request_status);
}
static int failed(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerReportV1*r,int own,int backend){
 first_failure(s,(uint32_t)own,backend);s->gate_enabled=0;
 /* The failed/partial path consumes the same request gate. It never attempts
  * normal close after invalidation and cannot later obtain close success. */
 if(!s->stop_consumed){
  s->stop_consumed=1;
  (void)ngp_voice_service_v1_invalidate(s->contexts.service,NGP_VOICE_CANCELLED_V1);
 }
 request_once(s);return emit(s,r,(int)s->first_worker_status,1);
}
static int current(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerReportV1*r){
 NgpDeviceMixerFactsV1 f=facts_report();int code=ngp_audio_worker_broker_query_v1(s->broker,&f);
 if(code||!facts_valid(&f)||f.voice_service!=s->facts.voice_service||f.intent_epoch!=s->facts.intent_epoch||
  f.sample_rate!=s->facts.sample_rate||f.channels!=s->facts.channels||f.maximum_frames!=s->facts.maximum_frames)
  return failed(s,r,NGP_AUDIO_WORKER_BROKER,code?code:NGP_DEVICE_BROKER_ROUTE_V1);
 return 0;
}
static int prepare_tokens(NgpAudioWorkerRuntimeV1*s,uint32_t kind,NgpVoiceTokensV1**out,uint32_t*count,NgpAudioWorkerReportV1*r){
 NgpVoiceTokensReportV1 t=token_report();int code=ngp_voice_tokens_v1_reserve(kind,out,&t);
 if(code||!*out)return failed(s,r,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_TOKENS_STATE);
 t=token_report();code=ngp_voice_tokens_v1_prepare(*out,s->contexts.service,&t);
 /* Record acknowledged tracking even on a partially failed preparation. It is
  * diagnostic only and never used to release or retry a failed dependency. */
 if(t.worker_observation)*count=t.reserved_tracked;
 uint32_t capacity=kind==3?13:32;
 if(code||t.phase!=NGP_TOKENS_READY||t.kind!=kind||t.capacity!=capacity||t.reserved_tracked!=capacity||t.loans_tracked||!t.worker_observation)
  return failed(s,r,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_TOKENS_STATE);
 return 0;
}
int ngp_audio_worker_v1_open(void*broker,NgpAudioWorkerRuntimeV1**out,NgpAudioWorkerReportV1*r){
 if(!valid(r))return NGP_AUDIO_WORKER_ARGUMENT;
 if(!broker||!out||*out)return emit(NULL,r,NGP_AUDIO_WORKER_ARGUMENT,0);
 if(pthread_main_np())return emit(NULL,r,NGP_AUDIO_WORKER_OWNER,0);
 if(atomic_load_explicit(&canonical,memory_order_acquire))return emit(NULL,r,NGP_AUDIO_WORKER_STATE,0);
 NgpDeviceMixerFactsV1 f=facts_report();int code=ngp_audio_worker_broker_query_v1(broker,&f);
 if(code==NGP_DEVICE_BROKER_PENDING_V1)return emit(NULL,r,NGP_AUDIO_WORKER_PENDING,0);
 if(code||!facts_valid(&f))return emit(NULL,r,NGP_AUDIO_WORKER_BROKER,0);
 NgpAudioWorkerRuntimeV1*s=calloc(1,sizeof *s);if(!s)return emit(NULL,r,NGP_AUDIO_WORKER_ALLOC,0);
 s->owner=pthread_self();s->broker=broker;s->facts=f;s->contexts.service=f.voice_service;atomic_init(&s->phase,NGP_AUDIO_WORKER_STARTING);
 NgpAudioWorkerRuntimeV1*empty=NULL;
 if(!atomic_compare_exchange_strong_explicit(&canonical,&empty,s,memory_order_acq_rel,memory_order_acquire)){free(s);return emit(NULL,r,NGP_AUDIO_WORKER_STATE,0);}
 *out=s;NgpVoiceReportV1 v=voice_report();code=ngp_voice_service_v1_prepare(s->contexts.service,&v);
 if(code||v.phase!=NGP_VOICE_READY_GATED_V1)return failed(s,r,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_VOICE_STATE_V1);
 s->voice_prepared=1;
 if((code=prepare_tokens(s,1,&s->contexts.ordinary,&s->ordinary_reserved,r)))return code;
 NgpPositionalReportV1 p=positional_report();code=ngp_positional_v1_open(s->contexts.service,&s->contexts.positional,&p);
 if(!code&&p.phase==NGP_POSITIONAL_LIVE&&p.capacity==32&&!p.active&&p.worker_observation&&s->contexts.positional)s->positional_reserved=32;
 else return failed(s,r,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_POSITIONAL_STATE);
 if((code=prepare_tokens(s,3,&s->contexts.streams,&s->stream_reserved,r)))return code;
 if((code=current(s,r)))return code;
 v=voice_report();code=ngp_voice_service_v1_enable(s->contexts.service,&v);
 if(code||v.phase!=NGP_VOICE_READY_OPEN_V1)return failed(s,r,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_VOICE_STATE_V1);
 s->gate_enabled=1;
 if((code=current(s,r)))return code;
 atomic_store_explicit(&s->phase,NGP_AUDIO_WORKER_READY,memory_order_release);return emit(s,r,0,1);
}
int ngp_audio_worker_v1_borrow(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerContextsV1*out,NgpAudioWorkerReportV1*r){
 if(!valid(r))return NGP_AUDIO_WORKER_ARGUMENT;
 if(!known(s)||!out)return emit(NULL,r,NGP_AUDIO_WORKER_ARGUMENT,0);
 if(!pthread_equal(s->owner,pthread_self()))return emit(s,r,NGP_AUDIO_WORKER_OWNER,0);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)!=NGP_AUDIO_WORKER_READY)return emit(s,r,NGP_AUDIO_WORKER_RETAINED,1);
 if(out->service||out->ordinary||out->positional||out->streams)return emit(s,r,NGP_AUDIO_WORKER_ARGUMENT,1);
 int code=current(s,r);if(code)return code;
 *out=s->contexts;return emit(s,r,0,1);
}

static int stop_valid(const NgpAudioWorkerStopReportV1*r){return r&&r->version==1&&r->size==sizeof *r;}
static int stop_emit(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r,int code,int worker){
 NgpAudioWorkerStopReportV1 v={0};v.version=1;v.size=sizeof v;v.status=(uint32_t)code;
 if(s)v.phase=atomic_load_explicit(&s->phase,memory_order_acquire);
 if(worker){
  v.worker_observation=1;v.stop_consumed=s->stop_consumed;v.close_attempted=s->close_attempted;v.close_ack=s->close_ack;
  v.request_returned=s->stop_request_returned;v.submission_status=s->stop_request_status;
  v.broker_outcome=s->broker_outcome;v.first_worker_status=s->first_worker_status;
  v.backend_status=s->backend_status;v.intent_epoch=s->facts.intent_epoch;
 }
 *r=v;return code;
}
static int stop_owner(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 if(!known(s))return stop_emit(NULL,r,NGP_AUDIO_WORKER_ARGUMENT,0);
 if(pthread_main_np()||!pthread_equal(s->owner,pthread_self()))return stop_emit(s,r,NGP_AUDIO_WORKER_OWNER,0);
 return 0;
}
/* Observation never commits STOPPED. Wait adds fresh clock/token admission. */
static int observe_stop_owned(NgpAudioWorkerRuntimeV1*s){
 if(s->first_worker_status)return (int)s->first_worker_status;
 if(!s->stop_consumed||atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_AUDIO_WORKER_STARTING)return NGP_AUDIO_WORKER_STATE;
 NgpDeviceMixerStopObservationV1 o={0};o.version=1;o.size=sizeof o;
 int code=ngp_audio_worker_broker_query_stop_v1(s->broker,&o);
 if(code||o.version!=1||o.size!=sizeof o||o.reserved||o.voice_service!=s->contexts.service||o.intent_epoch!=s->facts.intent_epoch||o.outcome>NGP_DEVICE_STOP_FAILED_V1){
  first_failure(s,NGP_AUDIO_WORKER_BROKER,code?code:NGP_DEVICE_BROKER_ROUTE_V1);
 }else{
  s->broker_outcome=o.outcome;
  if(o.outcome==NGP_DEVICE_STOP_FAILED_V1)first_failure(s,NGP_AUDIO_WORKER_BROKER,NGP_DEVICE_BROKER_CLOSED_V1);
 }
 if(s->first_worker_status)return (int)s->first_worker_status;
 return s->close_ack&&s->broker_outcome==NGP_DEVICE_STOP_ACK_V1?0:NGP_AUDIO_WORKER_PENDING;
}
static void commit_stop_owned(NgpAudioWorkerRuntimeV1*s){
 atomic_store_explicit(&s->phase,NGP_AUDIO_WORKER_STOPPED_RETAINED,memory_order_release);
}
static int poll_stop_owned(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 if(s->first_worker_status)return stop_emit(s,r,(int)s->first_worker_status,1);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_AUDIO_WORKER_STOPPED_RETAINED)return stop_emit(s,r,0,1);
 int code=observe_stop_owned(s);if(!code)commit_stop_owned(s);
 return stop_emit(s,r,code,1);
}
static void start_stop_owned(NgpAudioWorkerRuntimeV1*s){
 if(s->first_worker_status||s->stop_consumed||atomic_load_explicit(&s->phase,memory_order_acquire)!=NGP_AUDIO_WORKER_READY)return;
 s->stop_consumed=1;s->gate_enabled=0;
 atomic_store_explicit(&s->phase,NGP_AUDIO_WORKER_STOPPING_RETAINED,memory_order_release);
 s->close_attempted=1;NgpVoiceReportV1 v=voice_report();
 int code=ngp_voice_service_v1_close(s->contexts.service,&v);
 if(!code&&v.version==1&&v.size==sizeof v&&!v.status&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1)s->close_ack=1;
 else first_failure(s,NGP_AUDIO_WORKER_BACKEND,code?code:NGP_VOICE_STATE_V1);
 request_once(s);
}
int ngp_audio_worker_v1_request_stop(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 if(!stop_valid(r))return NGP_AUDIO_WORKER_ARGUMENT;
 int code=stop_owner(s,r);if(code)return code;
 if(s->first_worker_status)return stop_emit(s,r,(int)s->first_worker_status,1);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_AUDIO_WORKER_STARTING)return stop_emit(s,r,NGP_AUDIO_WORKER_STATE,1);
 start_stop_owned(s);return poll_stop_owned(s,r);
}
int ngp_audio_worker_v1_poll_stop(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 if(!stop_valid(r))return NGP_AUDIO_WORKER_ARGUMENT;
 int code=stop_owner(s,r);if(code)return code;
 return poll_stop_owned(s,r);
}

/* First reason is private retained diagnostic; old64B report ABI is unchanged. */
enum {WAIT_CLOCK=1,WAIT_BACKWARD=2,WAIT_DEADLINE=3,WAIT_ATTEMPTS=4,WAIT_SLEEP=5,WAIT_ADMISSION=6,WAIT_OVERFLOW=7};
static int wait_fault(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r,uint32_t reason,uint32_t provider,int error,int admitted){
 if(!s->wait_reason){s->wait_reason=reason;s->wait_detail_status=provider;s->wait_native_error=(reason==WAIT_CLOCK||reason==WAIT_SLEEP)?error:0;}
 first_failure(s,NGP_AUDIO_WORKER_WAIT,error);s->gate_enabled=0;
 if(!s->stop_consumed){s->stop_consumed=1;(void)ngp_voice_service_v1_invalidate(s->contexts.service,NGP_VOICE_CANCELLED_V1);}
 /* Separate started gate allows one cancellation after consumed=1. A failed
  * queue admission never grants permission to submit or synchronously wait. */
 if(admitted)request_once(s);
 return stop_emit(s,r,(int)s->first_worker_status,1);
}
static int wait_sample(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r,int admitted){
 uint64_t now=0;int32_t error=0;int code=ngp_audio_stop_os_now_v1(&now,&error);
 if(code)return wait_fault(s,r,WAIT_CLOCK,(uint32_t)code,error,admitted);
 if(now<s->wait_last)return wait_fault(s,r,WAIT_BACKWARD,0,0,admitted);
 s->wait_last=now;
 if(now>=s->wait_deadline)return wait_fault(s,r,WAIT_DEADLINE,0,0,admitted);
 return 0;
}
int ngp_audio_worker_v1_wait_stop(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 if(!stop_valid(r))return NGP_AUDIO_WORKER_ARGUMENT;
 int code=stop_owner(s,r);if(code)return code;
 if(s->first_worker_status)return stop_emit(s,r,(int)s->first_worker_status,1);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_AUDIO_WORKER_STARTING)return stop_emit(s,r,NGP_AUDIO_WORKER_STATE,1);
 uint64_t start=0;int32_t error=0;code=ngp_audio_stop_os_now_v1(&start,&error);
 if(code)return wait_fault(s,r,WAIT_CLOCK,(uint32_t)code,error,0);
 if(start>UINT64_MAX-UINT64_C(5000000000))return wait_fault(s,r,WAIT_OVERFLOW,0,0,0);
 s->wait_start=start;s->wait_last=start;s->wait_deadline=start+UINT64_C(5000000000);s->wait_attempts=0;
 code=ngp_audio_worker_broker_wait_admit_v1(s->broker,s->contexts.service,s->facts.intent_epoch);
 if(code)return wait_fault(s,r,WAIT_ADMISSION,(uint32_t)code,code,0);
 if((code=wait_sample(s,r,1)))return code;
 start_stop_owned(s);
 if(s->first_worker_status)return stop_emit(s,r,(int)s->first_worker_status,1);
 uint32_t unclaimed=4096,current_token_live=0;
 while(unclaimed){
  /* unclaimed0 does not revoke the final current token. It remains live until
   * this cycle returns or retires after its one wait, including EINTR. */
  unclaimed--;current_token_live=1;s->wait_attempts++;
  if((code=wait_sample(s,r,1))){current_token_live=0;return code;}
  if(!current_token_live)return wait_fault(s,r,WAIT_ATTEMPTS,0,0,1);
  int observed=observe_stop_owned(s);
  if(s->first_worker_status){current_token_live=0;return stop_emit(s,r,(int)s->first_worker_status,1);}
  if(observed!=0&&observed!=NGP_AUDIO_WORKER_PENDING){current_token_live=0;return stop_emit(s,r,observed,1);}
  if((code=wait_sample(s,r,1))){current_token_live=0;return code;}
  if(!observed){if(!current_token_live)return wait_fault(s,r,WAIT_ATTEMPTS,0,0,1);commit_stop_owned(s);current_token_live=0;return stop_emit(s,r,0,1);}
  uint64_t remaining=s->wait_deadline-s->wait_last;
  uint64_t interval=remaining<UINT64_C(2000000)?remaining:UINT64_C(2000000);
  if(!current_token_live)return wait_fault(s,r,WAIT_ATTEMPTS,0,0,1);
  error=0;code=ngp_audio_stop_os_sleep_v1(interval,&error);current_token_live=0;
  if(code!=NGP_STOP_OS_OK_V1&&code!=NGP_STOP_OS_INTERRUPTED_V1)return wait_fault(s,r,WAIT_SLEEP,(uint32_t)code,error,1);
 }
 return wait_fault(s,r,WAIT_ATTEMPTS,0,0,1);
}
