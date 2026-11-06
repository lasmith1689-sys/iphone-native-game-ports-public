/* OWN real OS-clock+real MA voice/runtime smoke. Broker ONLY is synthetic. */
#include "ngp_audio_worker_runtime_v1.h"
#include "ngp_audio_stop_os_v1.h"
#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
static NgpVoiceServiceV1*service;
static NgpAudioWorkerRuntimeV1*runtime;
static _Atomic unsigned checks;
static unsigned requests,queries;
static uint64_t worker_stack,start,end;
static void check(int v,int line){atomic_fetch_add(&checks,1);if(!v){fprintf(stderr,"{\"status\":\"real-os-smoke-failed\",\"line\":%d}\n",line);exit(101);}}
#define CHECK(v)check(!!(v),__LINE__)
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioWorkerStopReportV1 sr(void){NgpAudioWorkerStopReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
int ngp_audio_worker_broker_query_v1(void*b,NgpDeviceMixerFactsV1*out){CHECK(b==&service);*out=(NgpDeviceMixerFactsV1){1,sizeof *out,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,service};return 0;}
int ngp_audio_worker_broker_wait_admit_v1(void*b,NgpVoiceServiceV1*s,uint64_t epoch){CHECK(b==&service&&s==service&&epoch==7);return 0;}
int ngp_audio_worker_broker_request_stop_v1(void*b){CHECK(b==&service&&++requests==1);NgpVoiceReportV1 r=vr();CHECK(ngp_voice_service_v1_snapshot(service,0,&r)==NGP_VOICE_CLOSED_V1&&r.phase==NGP_VOICE_CLOSED_RETAINED_V1);CHECK(!ngp_voice_service_v1_invalidate(service,NGP_VOICE_CANCELLED_V1));return 0;}
int ngp_audio_worker_broker_query_stop_v1(void*b,NgpDeviceMixerStopObservationV1*out){CHECK(b==&service);queries++;*out=(NgpDeviceMixerStopObservationV1){1,sizeof *out,queries>=2?NGP_DEVICE_STOP_ACK_V1:NGP_DEVICE_STOP_PENDING_V1,0,7,service};return 0;}
static void*worker(void*p){
 (void)p;CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);NgpAudioWorkerReportV1 w={0};w.version=1;w.size=sizeof w;CHECK(!ngp_audio_worker_v1_open(&service,&runtime,&w));int32_t error=0;CHECK(!ngp_audio_stop_os_now_v1(&start,&error));NgpAudioWorkerStopReportV1 r=sr();CHECK(!ngp_audio_worker_v1_wait_stop(runtime,&r)&&r.close_ack&&r.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED&&!r.first_worker_status);CHECK(!ngp_audio_stop_os_now_v1(&end,&error)&&end>=start&&queries==2&&requests==1);r=sr();CHECK(!ngp_audio_worker_v1_wait_stop(runtime,&r)&&queries==3&&requests==1);return NULL;
}
int main(void){
 CHECK(pthread_main_np());NgpVoiceOutputV1 output={1,sizeof output,48000,2};NgpVoiceReportV1 r=vr();CHECK(!ngp_voice_service_v1_reserve(&output,&service,&r));pthread_attr_t attr;CHECK(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,8388608));pthread_t t;CHECK(!pthread_create(&t,&attr,worker,NULL));CHECK(!pthread_join(t,NULL)&&!pthread_attr_destroy(&attr));
 printf("{\"status\":\"real-os-wait-smoke-passed\",\"checks\":%u,\"queries\":%u,\"requests\":%u,\"startNs\":%llu,\"endNs\":%llu,\"workerStackBytes\":%llu,\"realMonotonicProviderLinked\":true,\"brokerIsSynthetic\":true,\"hardWallDeadlineClaim\":false,\"device\":false,\"game\":false}\n",atomic_load(&checks),queries,requests,(unsigned long long)start,(unsigned long long)end,(unsigned long long)worker_stack);return 0;
}
