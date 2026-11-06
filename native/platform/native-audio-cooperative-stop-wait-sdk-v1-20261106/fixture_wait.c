/* OWN actual-C new API tests. Real backend/runtime; broker and OS providers
 * are explicitly SYNTHETIC. No UIKit, device, source guest or physical claim. */
#include "ngp_audio_worker_runtime_v1.h"
#include "ngp_audio_stop_terminal_v1.h"
#include "test_wait_provider.h"
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static _Atomic unsigned checks;
static unsigned mode,requests,closed_seen,starting_refusals,ready,main_done;
static uint64_t voice_handle,worker_stack;
static int final_status;static uint32_t final_phase,final_first;static int32_t final_backend,final_submission;
static NgpVoiceServiceV1*service;
static NgpAudioWorkerRuntimeV1*runtime;
static NgpAudioStopTerminalV1 terminal;
static max_align_t foreign_identity;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static void check(int v,const char*id,int line){atomic_fetch_add(&checks,1);if(!v){fprintf(stderr,"{\"status\":\"wait-assertion-failed\",\"id\":\"%s\",\"line\":%d}\n",id,line);exit(101);}}
#define CHECK(v)check(!!(v),"setup-or-protocol",__LINE__)
#define ID(v,id)check(!!(v),(id),__LINE__)
static NgpAudioWorkerStopReportV1 sr(void){NgpAudioWorkerStopReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioWorkerReportV1 wr(void){NgpAudioWorkerReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 tr(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
int ngp_audio_worker_broker_query_v1(void*b,NgpDeviceMixerFactsV1*out){
 static unsigned count;CHECK(b==&service&&out->version==1&&out->size==sizeof *out);count++;
 if(mode==30&&count==2){unsigned n=ngp_wait_test.now_calls;NgpAudioWorkerStopReportV1 s=sr();CHECK(ngp_audio_worker_v1_wait_stop(runtime,&s)==NGP_AUDIO_WORKER_STATE&&s.phase==NGP_AUDIO_WORKER_STARTING&&!s.stop_consumed&&ngp_wait_test.now_calls==n);starting_refusals++;}
 if((mode==22||mode==24)&&count==2)return NGP_DEVICE_BROKER_ROUTE_V1;
 *out=(NgpDeviceMixerFactsV1){1,sizeof *out,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,service};return 0;
}
int ngp_audio_worker_broker_wait_admit_v1(void*b,NgpVoiceServiceV1*s,uint64_t epoch){
 CHECK(b==&service&&s==service&&epoch==7);ngp_wait_test.admissions++;
 if(mode==18)return NGP_DEVICE_BROKER_ROUTE_V1;
 if(mode==26)return NGP_DEVICE_BROKER_CLOSED_V1;
 return 0;
}
int ngp_audio_worker_broker_request_stop_v1(void*b){
 CHECK(b==&service);requests++;CHECK(requests==1);NgpVoiceReportV1 v=vr();int code=ngp_voice_service_v1_snapshot(service,voice_handle,&v);
 if(mode==22||mode==23||mode==24||mode==28){CHECK(code&&v.phase==NGP_VOICE_FAILED_RETAINED_V1);}
 else{CHECK(code==NGP_VOICE_CLOSED_V1&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1&&v.owned_pcm_bytes==256);closed_seen++;}
 CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));return mode==34?NGP_DEVICE_BROKER_PENDING_V1:0;
}
int ngp_audio_worker_broker_query_stop_v1(void*b,NgpDeviceMixerStopObservationV1*out){
 CHECK(b==&service&&out->version==1&&out->size==sizeof *out);ngp_wait_test.queries++;
 unsigned q=ngp_wait_test.queries;
 if(mode==21)return NGP_DEVICE_BROKER_EXCEPTION_V1;
 int pending=mode==2||mode==4||mode==5||mode==6||mode==7||mode==8||mode==9||mode==10||(mode==3&&q<4096)||((mode==32||mode==34)&&q<3);
 if(!pending)CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,NGP_DEVICE_STOP_ACK_V1)==NGP_DEVICE_STOP_ACK_V1);
 if(mode==1||mode==2)ngp_wait_test.now=ngp_wait_test.deadline;
 if(mode==7)ngp_wait_test.now=ngp_wait_test.deadline-1;
 if(mode==12)ngp_wait_test.fail_next_now=1;
 if(mode==15)ngp_wait_test.now=99;
 *out=(NgpDeviceMixerStopObservationV1){1,sizeof *out,ngp_audio_stop_terminal_v1_read(&terminal),0,7,service};
 if(mode==19)out->voice_service=(NgpVoiceServiceV1*)&foreign_identity;
 if(mode==20)out->intent_epoch=11;
 return 0;
}
static void*foreign(void*p){(void)p;unsigned n=ngp_wait_test.now_calls;NgpAudioWorkerStopReportV1 r=sr();CHECK(ngp_audio_worker_v1_wait_stop(runtime,&r)==NGP_AUDIO_WORKER_OWNER&&!r.worker_observation&&ngp_wait_test.now_calls==n);return NULL;}
static void prepare(void){
 CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);
 NgpAudioWorkerReportV1 w=wr();int rc=ngp_audio_worker_v1_open(&service,&runtime,&w);CHECK(runtime);
 if(mode==22||mode==24){CHECK(rc==NGP_AUDIO_WORKER_BROKER&&w.phase==NGP_AUDIO_WORKER_FAILED_RETAINED&&requests==1);return;}
 CHECK(!rc&&w.phase==NGP_AUDIO_WORKER_READY);NgpAudioWorkerContextsV1 c={0};w=wr();CHECK(!ngp_audio_worker_v1_borrow(runtime,&c,&w));
 uint32_t token=0;NgpVoiceTokensReportV1 t=tr();CHECK(!ngp_voice_tokens_v1_assign(c.ordinary,&token,&t));t=tr();CHECK(!ngp_voice_tokens_v1_resolve(c.ordinary,token,&voice_handle,&t)&&voice_handle);
 int16_t pcm[128];for(unsigned i=0;i<128;i++)pcm[i]=(int16_t)(i*16);NgpVoicePcmV1 clip={1,sizeof clip,NGP_VOICE_PCM_S16_V1,2,48000,0,pcm,sizeof pcm,64};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_bind_pcm(service,voice_handle,&clip,&v)&&v.owned_pcm_bytes==256);
}
static int positive(void){return mode==0||mode==3||mode==16||mode==17||mode==25||mode==30||mode==32||mode==34;}
static void*worker(void*p){
 (void)p;prepare();CHECK(!pthread_mutex_lock(&lock));ready=1;CHECK(!pthread_cond_signal(&condition));while(!main_done)CHECK(!pthread_cond_wait(&condition,&lock));CHECK(!pthread_mutex_unlock(&lock));
 if(mode==17){pthread_t t;CHECK(!pthread_create(&t,NULL,foreign,NULL)&&!pthread_join(t,NULL));}
 if(mode==31){NgpAudioWorkerStopReportV1 r=sr();final_status=ngp_audio_worker_v1_wait_stop(NULL,&r);CHECK(final_status==NGP_AUDIO_WORKER_ARGUMENT&&!ngp_wait_test.now_calls);return NULL;}
 if(mode==23)CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));
 if(mode==29){NgpAudioWorkerStopReportV1 r=sr();CHECK(!ngp_audio_worker_v1_request_stop(runtime,&r)&&r.close_ack&&r.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED);ngp_wait_test.fail_next_now=1;}
 NgpAudioWorkerStopReportV1 r=sr();int code=ngp_audio_worker_v1_wait_stop(runtime,&r);
 if(positive()){
  ID(!code&&r.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED&&r.close_ack&&!r.first_worker_status,"wait-matching-ACK-commit");
  if(mode==3)CHECK(ngp_wait_test.queries==4096&&ngp_wait_test.sleeps==4095);
  if(mode==32||mode==34)CHECK(ngp_wait_test.queries==3&&ngp_wait_test.sleeps==2);
  if(mode==34)CHECK(r.submission_status==NGP_DEVICE_BROKER_PENDING_V1&&requests==1&&!r.first_worker_status);
  if(mode==25){r=sr();CHECK(!ngp_audio_worker_v1_wait_stop(runtime,&r)&&requests==1&&closed_seen==1);}
  if(mode==30)CHECK(starting_refusals==1);
 }else{
  uint32_t expected=mode==19||mode==20||mode==21||mode==22||mode==24?NGP_AUDIO_WORKER_BROKER:mode==23?NGP_AUDIO_WORKER_BACKEND:NGP_AUDIO_WORKER_WAIT;
  ID(code==(int)expected&&r.first_worker_status==expected&&r.phase==NGP_AUDIO_WORKER_FAILED_RETAINED,"wait-fault-sticky-in-runtime");
  if(mode==1)ID(r.close_ack&&ngp_wait_test.queries==1&&!ngp_wait_test.sleeps,"late-ACK-no-STOPPED");
  if(mode==2)CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,1)==1);
  if(mode==4||mode==5||mode==6)CHECK(ngp_wait_test.queries==4096&&ngp_wait_test.sleeps==4096&&ngp_wait_test.now==100);
  if(mode==7)CHECK(ngp_wait_test.sleeps==1&&ngp_wait_test.last_interval==1);
  if(mode==8)CHECK(ngp_wait_test.sleeps==1&&ngp_wait_test.queries==1);
  if(mode==9||mode==11||mode==12||mode==27||mode==28||mode==29)CHECK(r.backend_status==EIO);
  if(mode==11||mode==13||mode==14||mode==27)CHECK(!requests&&!r.close_ack&&!ngp_wait_test.admissions&&!ngp_wait_test.sleeps);
  if(mode==18||mode==26)CHECK(!requests&&!r.close_ack&&!ngp_wait_test.sleeps);
  if(mode==33)CHECK(!ngp_wait_test.queries&&!ngp_wait_test.sleeps&&requests==1&&r.close_ack);
  if(mode==28)CHECK(requests==1&&!r.close_ack&&!closed_seen&&!ngp_wait_test.queries);
  if(mode==22||mode==24)CHECK(r.backend_status==NGP_DEVICE_BROKER_ROUTE_V1&&!ngp_wait_test.now_calls&&!ngp_wait_test.admissions);
  unsigned q=ngp_wait_test.queries,n=ngp_wait_test.now_calls,a=requests;int backend=r.backend_status;
  r=sr();CHECK(ngp_audio_worker_v1_poll_stop(runtime,&r)==(int)expected&&r.first_worker_status==expected&&r.backend_status==backend);
  r=sr();CHECK(ngp_audio_worker_v1_request_stop(runtime,&r)==(int)expected);
  r=sr();CHECK(ngp_audio_worker_v1_wait_stop(runtime,&r)==(int)expected);
  ID(ngp_wait_test.queries==q&&ngp_wait_test.now_calls==n&&requests==a,"fault-no-query-retry-or-promotion");
 }
 CHECK(ngp_wait_test.maximum_interval<=2000000&&requests<=1);final_status=code;final_phase=r.phase;final_first=r.first_worker_status;final_backend=r.backend_status;final_submission=r.submission_status;return NULL;
}
int main(int argc,char**argv){
 CHECK(pthread_main_np()&&argc==2);char*end=NULL;unsigned long n=strtoul(argv[1],&end,10);CHECK(end&&!*end&&n<=34);mode=(unsigned)n;ngp_wait_test.mode=mode;ngp_wait_test.now=100;ngp_wait_test.deadline=UINT64_C(5000000100);atomic_init(&terminal.outcome,0);CHECK(sizeof(NgpAudioWorkerStopReportV1)==64);
 NgpVoiceOutputV1 output={1,sizeof output,48000,2};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_reserve(&output,&service,&v));pthread_attr_t attr;CHECK(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,8388608));pthread_t t;CHECK(!pthread_create(&t,&attr,worker,NULL));
 CHECK(!pthread_mutex_lock(&lock));while(!ready)CHECK(!pthread_cond_wait(&condition,&lock));if(mode==16){unsigned before=ngp_wait_test.now_calls;NgpAudioWorkerStopReportV1 r=sr();CHECK(ngp_audio_worker_v1_wait_stop(runtime,&r)==NGP_AUDIO_WORKER_OWNER&&!r.worker_observation&&ngp_wait_test.now_calls==before);}main_done=1;CHECK(!pthread_cond_signal(&condition)&&!pthread_mutex_unlock(&lock));CHECK(!pthread_join(t,NULL)&&!pthread_attr_destroy(&attr));
 printf("{\"status\":\"new-wait-control-passed\",\"mode\":%u,\"checks\":%u,\"queries\":%u,\"sleeps\":%u,\"requests\":%u,\"clockCalls\":%u,\"finalStatus\":%d,\"finalPhase\":%u,\"firstWorkerStatus\":%u,\"backendStatus\":%d,\"submissionStatus\":%d,\"workerStackBytes\":%llu,\"brokerIsSynthetic\":true,\"clockIsSynthetic\":true,\"actualVoiceRuntimeLinked\":true,\"device\":false,\"game\":false}\n",mode,atomic_load(&checks),ngp_wait_test.queries,ngp_wait_test.sleeps,requests,ngp_wait_test.now_calls,final_status,final_phase,final_first,final_backend,final_submission,(unsigned long long)worker_stack);return 0;
}
