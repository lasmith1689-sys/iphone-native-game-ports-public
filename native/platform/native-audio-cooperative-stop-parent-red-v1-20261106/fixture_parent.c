/* OWN future parent-contract counterexamples. Actual voice/runtime/terminal C;
 * three broker functions and the external clock are explicitly SYNTHETIC.
 * No new wait function, OS clock claim, device/session or guest entry. */
#include "ngp_audio_worker_runtime_v1.h"
#include "ngp_audio_stop_terminal_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NgpVoiceServiceV1 *service;
static NgpAudioWorkerRuntimeV1 *runtime;
static NgpAudioStopTerminalV1 terminal;
static uint64_t voice_handle,logical_now,deadline;
static _Atomic unsigned checks;
static unsigned mode,requests,queries,closed_seen;
static uint64_t worker_stack;
static void check(int value,const char *id,int line){
 atomic_fetch_add_explicit(&checks,1,memory_order_relaxed);
 if(!value){fprintf(stderr,"{\"status\":\"fixture-assertion-failed\",\"id\":\"%s\",\"line\":%d}\n",id,line);exit(101);}
}
#define CHECK(v)check(!!(v),"setup-or-baseline",__LINE__)
#define EXPECT(v,id)check(!!(v),(id),__LINE__)
static NgpAudioWorkerReportV1 wr(void){NgpAudioWorkerReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioWorkerStopReportV1 sr(void){NgpAudioWorkerStopReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 tr(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}

/* Exactly the old Mac diagnostic boundary: never production ready/ACK claims. */
int ngp_audio_worker_broker_query_v1(void *broker,NgpDeviceMixerFactsV1 *out){
 CHECK(broker==&service&&out->version==1&&out->size==sizeof *out);
 *out=(NgpDeviceMixerFactsV1){1,sizeof *out,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,service};return 0;
}
int ngp_audio_worker_broker_request_stop_v1(void *broker){
 CHECK(broker==&service);requests++;CHECK(requests==1);
 NgpVoiceReportV1 v=vr();
 CHECK(ngp_voice_service_v1_snapshot(service,voice_handle,&v)==NGP_VOICE_CLOSED_V1);
 CHECK(v.phase==NGP_VOICE_CLOSED_RETAINED_V1&&v.owned_pcm_bytes==256);closed_seen++;
 CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));
 return 0; /* Synthetic successful submission, explicitly not stop ACK. */
}
int ngp_audio_worker_broker_query_stop_v1(void *broker,NgpDeviceMixerStopObservationV1 *out){
 CHECK(broker==&service&&out->version==1&&out->size==sizeof *out);queries++;
 *out=(NgpDeviceMixerStopObservationV1){1,sizeof *out,ngp_audio_stop_terminal_v1_read(&terminal),0,7,service};
 if(mode==1)logical_now=deadline; /* Explicit synthetic time crossing during the actual query. */
 return 0;
}
static void prepare_owned(void){
 CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);
 NgpAudioWorkerReportV1 w=wr();CHECK(!ngp_audio_worker_v1_open(&service,&runtime,&w));
 CHECK(runtime&&w.phase==NGP_AUDIO_WORKER_READY&&w.ordinary_reserved==32&&w.positional_reserved==32&&w.stream_reserved==13);
 NgpAudioWorkerContextsV1 contexts={0};w=wr();CHECK(!ngp_audio_worker_v1_borrow(runtime,&contexts,&w)&&contexts.service==service);
 NgpVoiceTokensReportV1 t=tr();uint32_t token=0;CHECK(!ngp_voice_tokens_v1_assign(contexts.ordinary,&token,&t)&&token);
 t=tr();CHECK(!ngp_voice_tokens_v1_resolve(contexts.ordinary,token,&voice_handle,&t)&&voice_handle);
 int16_t pcm[128];for(unsigned i=0;i<128;i++)pcm[i]=(int16_t)(i*16);
 NgpVoicePcmV1 clip={1,sizeof clip,NGP_VOICE_PCM_S16_V1,2,48000,0,pcm,sizeof pcm,64};NgpVoiceReportV1 v=vr();
 CHECK(!ngp_voice_service_v1_bind_pcm(service,voice_handle,&clip,&v)&&v.owned_pcm_bytes==256);
 CHECK(sizeof(NgpAudioWorkerStopReportV1)==64);
}
static void observation(const NgpAudioWorkerStopReportV1 *r,int code,unsigned external_timeout){
 printf("{\"status\":\"parent-contract-observation\",\"mode\":%u,\"code\":%d,\"phase\":%u,\"firstWorkerStatus\":%u,\"closeAck\":%u,\"requests\":%u,\"queries\":%u,\"realCloseObserved\":%u,\"logicalNow\":%llu,\"deadline\":%llu,\"externalTimeoutRecorded\":%u,\"checks\":%u,\"workerStackBytes\":%llu,\"brokerIsSynthetic\":true,\"externalClockIsSynthetic\":true,\"actualVoiceRuntimeLinked\":true,\"physicalDevice\":false,\"game\":false}\n",mode,code,r->phase,r->first_worker_status,r->close_ack,requests,queries,closed_seen,(unsigned long long)logical_now,(unsigned long long)deadline,external_timeout,atomic_load(&checks),(unsigned long long)worker_stack);
 CHECK(!fflush(stdout));
}
static void *worker(void *unused){
 (void)unused;prepare_owned();logical_now=100;deadline=logical_now+UINT64_C(5000000000);
 if(mode==1){
  CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,NGP_DEVICE_STOP_ACK_V1)==NGP_DEVICE_STOP_ACK_V1);
  CHECK(logical_now<deadline);NgpAudioWorkerStopReportV1 r=sr();
  int code=ngp_audio_worker_v1_request_stop(runtime,&r);
  CHECK(logical_now==deadline&&queries==1&&requests==1&&closed_seen==1&&r.close_ack);
  observation(&r,code,0);
  /* This is the new wait contract, intentionally absent from old nonwaiting API. */
  EXPECT(code!=0&&r.phase!=NGP_AUDIO_WORKER_STOPPED_RETAINED,"late-ack-must-not-publish-stopped");
 }else{
  NgpAudioWorkerStopReportV1 r=sr();int code=ngp_audio_worker_v1_request_stop(runtime,&r);
  CHECK(code==NGP_AUDIO_WORKER_PENDING&&r.close_ack&&!r.first_worker_status&&queries==1&&requests==1);
  unsigned external_timeout=mode==2;
  if(external_timeout)logical_now=deadline; /* Deliberate observer-only record; no new runtime API is invented. */
  CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,NGP_DEVICE_STOP_ACK_V1)==NGP_DEVICE_STOP_ACK_V1);
  r=sr();code=ngp_audio_worker_v1_poll_stop(runtime,&r);CHECK(queries==2&&requests==1&&closed_seen==1&&r.close_ack);
  observation(&r,code,external_timeout);
  if(mode==2){
   CHECK(external_timeout&&logical_now==deadline);
   EXPECT(code!=0&&r.phase==NGP_AUDIO_WORKER_FAILED_RETAINED&&r.first_worker_status,"external-timeout-must-stay-worker-sticky");
  }else CHECK(!code&&r.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED&&!r.first_worker_status);
 }
 return NULL;
}
int main(int argc,char **argv){
 CHECK(pthread_main_np());CHECK(argc==2&&strlen(argv[1])==1&&argv[1][0]>='0'&&argv[1][0]<='2');mode=(unsigned)(argv[1][0]-'0');atomic_init(&terminal.outcome,0);
 NgpVoiceOutputV1 output={1,sizeof output,48000,2};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_reserve(&output,&service,&v)&&service);
 pthread_attr_t attr;CHECK(!pthread_attr_init(&attr));CHECK(!pthread_attr_setstacksize(&attr,8388608));pthread_t thread;
 CHECK(!pthread_create(&thread,&attr,worker,NULL));CHECK(!pthread_join(thread,NULL));CHECK(!pthread_attr_destroy(&attr));
 printf("{\"status\":\"parent-baseline-control-passed\",\"mode\":%u,\"checks\":%u}\n",mode,atomic_load(&checks));return 0;
}
