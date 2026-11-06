/* OWN future Mac fixture. Real MA voice/backend; broker APIs explicitly
 * SYNTHETIC. No actual UIKit/session/device is run by this program. */
#include "ngp_audio_catalog_v1.h"
#include "ngp_audio_stop_terminal_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static _Atomic unsigned checks;
static unsigned mode,queries,stop_queries,requests,starting_refusals,closed_seen;
static uint64_t worker_stack,voice_handle;
static NgpVoiceServiceV1*service;
static NgpAudioWorkerRuntimeV1*runtime;
static NgpAudioCatalogV1*catalog;
static NgpAudioWorkerContextsV1 contexts;
static NgpAudioStopTerminalV1 terminal;
static max_align_t foreign_identity;
static pthread_mutex_t handshake=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static unsigned worker_ready,main_done;
static void check(int v,const char*id,int line){
 atomic_fetch_add_explicit(&checks,1,memory_order_relaxed);
 if(!v){fprintf(stderr,"{\"status\":\"fixture-assertion-failed\",\"id\":\"%s\",\"line\":%d}\n",id,line);exit(101);}
}
#define CHECK(v)check(!!(v),"generic",__LINE__)
#define ID(v,id)check(!!(v),(id),__LINE__)
static NgpAudioWorkerReportV1 wr(void){NgpAudioWorkerReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioWorkerStopReportV1 sr(void){NgpAudioWorkerStopReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioCatalogReportV1 cr(void){NgpAudioCatalogReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
/* These THREE definitions are the explicitly synthetic Mac broker ONLY.
 * All voice/token/positional/catalog/worker/terminal definitions link actual C. */
int ngp_audio_worker_broker_query_v1(void*b,NgpDeviceMixerFactsV1*f){
 CHECK(b==&service&&f->version==1&&f->size==sizeof *f);queries++;
 if(mode==8&&queries==2){
  NgpAudioWorkerStopReportV1 s=sr();unsigned n=requests;
  CHECK(runtime&&ngp_audio_worker_v1_request_stop(runtime,&s)==NGP_AUDIO_WORKER_STATE);
  CHECK(s.phase==NGP_AUDIO_WORKER_STARTING&&!s.stop_consumed&&!s.close_attempted&&requests==n);starting_refusals++;
 }
 if(mode==5&&queries==2)return NGP_DEVICE_BROKER_ROUTE_V1;
 *f=(NgpDeviceMixerFactsV1){1,sizeof *f,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,service};return 0;
}
int ngp_audio_worker_broker_request_stop_v1(void*b){
 CHECK(b==&service);requests++;CHECK(requests==1);
 NgpVoiceReportV1 v=vr();int code=ngp_voice_service_v1_snapshot(service,voice_handle,&v);
 if(mode!=5&&mode!=13){
  ID(code==NGP_VOICE_CLOSED_V1&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1,"real-close-before-broker-revoke");
  CHECK(v.owned_pcm_bytes==256);closed_seen++;
 }else CHECK(code!=0&&v.phase==NGP_VOICE_FAILED_RETAINED_V1);
 CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));
 if(mode==1||mode==2)return NGP_DEVICE_BROKER_PENDING_V1; /* actual tested overlap RETURN, not a fake ACK */
 if(mode==3)return NGP_DEVICE_BROKER_CLOSED_V1; /* synthetic nil-queue submission result */
 if(mode==4)return NGP_DEVICE_BROKER_EXCEPTION_V1; /* synthetic enqueue exception result */
 return 0;
}
int ngp_audio_worker_broker_query_stop_v1(void*b,NgpDeviceMixerStopObservationV1*out){
 CHECK(b==&service&&out->version==1&&out->size==sizeof *out);stop_queries++;
 if(mode==10)return NGP_DEVICE_BROKER_EXCEPTION_V1;
 *out=(NgpDeviceMixerStopObservationV1){1,sizeof *out,ngp_audio_stop_terminal_v1_read(&terminal),0,7,service};
 if(mode==6)out->voice_service=(NgpVoiceServiceV1*)&foreign_identity;
 if(mode==7)out->intent_epoch=11;
 if(mode==11)out->outcome=3;
 if(mode==12)out->size--;
 return 0;
}
static void*foreign(void*unused){
 (void)unused;NgpAudioWorkerStopReportV1 r=sr();unsigned q=stop_queries,n=requests;
 CHECK(ngp_audio_worker_v1_request_stop(runtime,&r)==NGP_AUDIO_WORKER_OWNER&&!r.worker_observation);
 r=sr();CHECK(ngp_audio_worker_v1_poll_stop(runtime,&r)==NGP_AUDIO_WORKER_OWNER&&!r.worker_observation);
 CHECK(q==stop_queries&&n==requests);return NULL;
}
static void await_main(void){
 CHECK(!pthread_mutex_lock(&handshake));worker_ready=1;CHECK(!pthread_cond_signal(&condition));
 while(!main_done)CHECK(!pthread_cond_wait(&condition,&handshake));
 CHECK(!pthread_mutex_unlock(&handshake));
}
static int expected_failure(void){return mode==2||mode==3||mode==4||mode==5||mode==6||mode==7||mode>=10;}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);
 NgpAudioWorkerReportV1 w=wr();int code=ngp_audio_worker_v1_open(&service,&runtime,&w);CHECK(runtime);
 if(mode==5){CHECK(code==NGP_AUDIO_WORKER_BROKER&&w.phase==NGP_AUDIO_WORKER_FAILED_RETAINED&&requests==1);}
 else{
  CHECK(!code&&w.phase==NGP_AUDIO_WORKER_READY&&w.ordinary_reserved==32&&w.positional_reserved==32&&w.stream_reserved==13);
  CHECK(!ngp_audio_worker_v1_borrow(runtime,&contexts,&w));NgpAudioCatalogReportV1 c=cr();CHECK(!ngp_audio_catalog_v1_bind(runtime,&catalog,&c));
  uint32_t legacy=99,token=0;CHECK(!ngp_audio_catalog_v1_open_provider(catalog,4,&legacy,&c)&&legacy==0);
  CHECK(!ngp_audio_catalog_v1_acquire_ordinary(catalog,&token,&c)&&token);CHECK(!ngp_audio_catalog_v1_resolve_voice(catalog,token,&voice_handle,&c)&&voice_handle);
  int16_t pcm[128];for(unsigned i=0;i<128;i++)pcm[i]=(int16_t)(i*16);NgpVoicePcmV1 clip={1,sizeof clip,NGP_VOICE_PCM_S16_V1,2,48000,0,pcm,sizeof pcm,64};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_bind_pcm(service,voice_handle,&clip,&v)&&v.owned_pcm_bytes==256);
 }
 await_main();pthread_t other;CHECK(!pthread_create(&other,NULL,foreign,NULL)&&!pthread_join(other,NULL));
 if(mode==13)CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));
 NgpAudioWorkerStopReportV1 s=sr();code=ngp_audio_worker_v1_request_stop(runtime,&s);
 if(mode==5){CHECK(code==NGP_AUDIO_WORKER_BROKER&&s.first_worker_status==NGP_AUDIO_WORKER_BROKER&&s.backend_status==NGP_DEVICE_BROKER_ROUTE_V1&&!s.close_attempted&&!s.close_ack&&requests==1);}
 else if(mode==13){CHECK(code==NGP_AUDIO_WORKER_BACKEND&&s.backend_status==NGP_VOICE_CANCELLED_V1&&!s.close_ack&&s.close_attempted);}
 else if(mode==1||mode==2){ID(code==NGP_AUDIO_WORKER_PENDING&&!s.first_worker_status&&!s.backend_status&&s.close_ack&&s.submission_status==NGP_DEVICE_BROKER_PENDING_V1,"pending-submission-no-primary");}
 else if(expected_failure()){CHECK(code==NGP_AUDIO_WORKER_BROKER&&s.first_worker_status==NGP_AUDIO_WORKER_BROKER&&s.close_ack);}
 else CHECK(code==NGP_AUDIO_WORKER_PENDING&&!s.first_worker_status&&s.close_ack);
 CHECK(s.worker_observation&&s.stop_consumed&&s.request_returned&&requests==1);
 NgpAudioWorkerContextsV1 denied={0};w=wr();CHECK(ngp_audio_worker_v1_borrow(runtime,&denied,&w)==NGP_AUDIO_WORKER_RETAINED&&!denied.service);
 if(catalog){uint32_t out=0;NgpAudioCatalogReportV1 c=cr();CHECK(ngp_audio_catalog_v1_acquire_ordinary(catalog,&out,&c)==NGP_CATALOG_RUNTIME&&!out);}
 uint32_t first=s.first_worker_status;int backend=s.backend_status;
 if(mode==9){for(unsigned i=0;i<5;i++){s=sr();CHECK(ngp_audio_worker_v1_request_stop(runtime,&s)==NGP_AUDIO_WORKER_PENDING&&s.close_ack&&s.close_attempted&&requests==1);}}
 CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,mode==2?NGP_DEVICE_STOP_FAILED_V1:NGP_DEVICE_STOP_ACK_V1)==(mode==2?2u:1u));
 s=sr();code=ngp_audio_worker_v1_poll_stop(runtime,&s);
 if(expected_failure()){
  CHECK(code!=0&&s.phase==NGP_AUDIO_WORKER_FAILED_RETAINED&&s.first_worker_status);
  if(first)CHECK(s.first_worker_status==first&&s.backend_status==backend);
 }else CHECK(!code&&s.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED&&s.close_ack&&!s.first_worker_status);
 uint32_t final_first=s.first_worker_status;int final_backend=s.backend_status;
 CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_LIFECYCLE_V1)); /* late revoke, separate from terminal */
 CHECK(ngp_audio_stop_terminal_v1_read(&terminal)==(mode==2?2u:1u));
 for(unsigned i=0;i<3;i++){
  s=sr();code=ngp_audio_worker_v1_request_stop(runtime,&s);CHECK(requests==1&&s.first_worker_status==final_first&&s.backend_status==final_backend);
  if(expected_failure())CHECK(code!=0);else CHECK(!code&&s.close_ack&&s.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED);
 }
 if(mode==8)CHECK(starting_refusals==1);
 return NULL;
}
typedef struct {NgpAudioStopTerminalV1*terminal;uint32_t value,result;pthread_mutex_t*lock;pthread_cond_t*cond;unsigned*start;} Publisher;
static void*publish_thread(void*arg){
 Publisher*p=arg;CHECK(!pthread_main_np());CHECK(!pthread_mutex_lock(p->lock));while(!*p->start)CHECK(!pthread_cond_wait(p->cond,p->lock));CHECK(!pthread_mutex_unlock(p->lock));p->result=ngp_audio_stop_terminal_v1_publish(p->terminal,p->value);return NULL;
}
static void terminal_cases(void){
 if(mode==20){
  CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,1)==1);
  ID(ngp_audio_stop_terminal_v1_publish(&terminal,2)==1,"terminal-ack-survives-failure");
  _Atomic unsigned revoked;atomic_init(&revoked,0);atomic_store(&revoked,1);CHECK(atomic_load(&revoked)==1&&ngp_audio_stop_terminal_v1_read(&terminal)==1);
 }else if(mode==21){CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,2)==2);CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,1)==2);CHECK(ngp_audio_stop_terminal_v1_read(&terminal)==2);}
 else if(mode==23){CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,1)==1);CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,0)==0);CHECK(ngp_audio_stop_terminal_v1_publish(&terminal,3)==0);CHECK(ngp_audio_stop_terminal_v1_read(&terminal)==1);}
 else{
  CHECK(mode==22);
  for(unsigned i=0;i<32;i++){
   NgpAudioStopTerminalV1 t;atomic_init(&t.outcome,0);pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t cond=PTHREAD_COND_INITIALIZER;unsigned start=0;
   Publisher a={&t,1,0,&lock,&cond,&start},b={&t,2,0,&lock,&cond,&start};pthread_t x,y;CHECK(!pthread_create(&x,NULL,publish_thread,&a)&&!pthread_create(&y,NULL,publish_thread,&b));
   CHECK(!pthread_mutex_lock(&lock));start=1;CHECK(!pthread_cond_broadcast(&cond));CHECK(!pthread_mutex_unlock(&lock));CHECK(!pthread_join(x,NULL)&&!pthread_join(y,NULL));uint32_t result=ngp_audio_stop_terminal_v1_read(&t);CHECK((result==1||result==2)&&a.result==result&&b.result==result);CHECK(ngp_audio_stop_terminal_v1_publish(&t,result==1?2:1)==result);CHECK(!pthread_cond_destroy(&cond)&&!pthread_mutex_destroy(&lock));
  }
 }
}
int main(int argc,char**argv){
 CHECK(pthread_main_np());CHECK(argc==2);char*end=NULL;unsigned long v=strtoul(argv[1],&end,10);CHECK(end&&*end==0&&v<=23&&(v<=13||v>=20));mode=(unsigned)v;atomic_init(&terminal.outcome,0);
 if(mode>=20)terminal_cases();
 else{
  NgpVoiceOutputV1 f={1,sizeof f,48000,2};NgpVoiceReportV1 r=vr();CHECK(!ngp_voice_service_v1_reserve(&f,&service,&r)&&service);
  NgpAudioWorkerReportV1 w=wr();CHECK(ngp_audio_worker_v1_open(&service,&runtime,&w)==NGP_AUDIO_WORKER_OWNER&&!runtime);
  pthread_attr_t attr;CHECK(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,8388608));pthread_t thread;CHECK(!pthread_create(&thread,&attr,worker,NULL));
  CHECK(!pthread_mutex_lock(&handshake));while(!worker_ready)CHECK(!pthread_cond_wait(&condition,&handshake));CHECK(runtime);
  NgpAudioWorkerStopReportV1 s=sr();CHECK(ngp_audio_worker_v1_request_stop(runtime,&s)==NGP_AUDIO_WORKER_OWNER&&!s.worker_observation);s=sr();CHECK(ngp_audio_worker_v1_poll_stop(runtime,&s)==NGP_AUDIO_WORKER_OWNER&&!s.worker_observation);
  main_done=1;CHECK(!pthread_cond_signal(&condition)&&!pthread_mutex_unlock(&handshake));CHECK(!pthread_join(thread,NULL));CHECK(!pthread_attr_destroy(&attr));
 }
 printf("{\"status\":\"real-retained-stop-case-passed\",\"mode\":%u,\"checks\":%u,\"workerStackBytes\":%llu,\"requests\":%u,\"realCloseBeforeRevokeObservations\":%u,\"startingRefusals\":%u,\"raceIterations\":%u,\"actualVoiceBackendLinked\":true,\"realVoiceServiceExercised\":%s,\"brokerIsSynthetic\":true,\"physicalDeviceStarted\":false,\"iPhoneExecuted\":false,\"gameExecuted\":false}\n",mode,atomic_load(&checks),(unsigned long long)worker_stack,requests,closed_seen,starting_refusals,mode==22?32u:0u,mode<=13?"true":"false");return 0;
}
