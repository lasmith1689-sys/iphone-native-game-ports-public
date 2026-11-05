/* Actual headless voice backend integration; no device/guest/retail inputs. */
#include "ngp_voice_tokens_v1.h"
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NgpVoiceServiceV1*service;static NgpVoiceTokensV1*pools[3];
static uint32_t tokens[3][32];static uint64_t handles[77];static float output[512];
static unsigned checks;static uint64_t worker_stack;static int render_code;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"real voice-token assertion %u line%d\n",checks,__LINE__);exit(4);}}while(0)
static NgpVoiceReportV1 voice_report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 report(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static void*callback(void*unused){(void)unused;render_code=ngp_voice_service_v1_render(service,output,256,256);return NULL;}
static void render(void){pthread_t t;CHECK(!pthread_create(&t,NULL,callback,NULL)&&!pthread_join(t,NULL));CHECK(render_code==0);}
static void*foreign(void*unused){(void)unused;NgpVoiceTokensReportV1 r=report();uint64_t native=0;CHECK(ngp_voice_tokens_v1_resolve(pools[0],tokens[0][0],&native,&r)==NGP_TOKENS_OWNER&&native==0&&!r.worker_observation);CHECK(ngp_voice_tokens_v1_close(pools[0],&r)==NGP_TOKENS_OWNER&&!r.worker_observation);return NULL;}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8u*1024u*1024u);
 NgpVoiceReportV1 v=voice_report();CHECK(!ngp_voice_service_v1_prepare(service,&v));NgpVoiceTokensReportV1 r=report();unsigned count=0;
 for(unsigned kind=1;kind<=3;kind++){
  unsigned capacity=kind==3?13:32;CHECK(!ngp_voice_tokens_v1_prepare(pools[kind-1],service,&r));CHECK(r.reserved_tracked==capacity&&r.kind==kind&&r.phase==NGP_TOKENS_READY);
  for(unsigned i=0;i<capacity;i++){
   CHECK(!ngp_voice_tokens_v1_assign(pools[kind-1],&tokens[kind-1][i],&r));CHECK(tokens[kind-1][i]&&(tokens[kind-1][i]&3)==kind);
   uint64_t native=0;CHECK(!ngp_voice_tokens_v1_resolve(pools[kind-1],tokens[kind-1][i],&native,&r));CHECK(native&&r.voice.handle==native&&r.voice.kind==kind);
   for(unsigned j=0;j<count;j++)CHECK(native!=handles[j]);handles[count++]=native;
  }
  uint32_t extra=0;CHECK(ngp_voice_tokens_v1_assign(pools[kind-1],&extra,&r)==NGP_TOKENS_LIMIT&&extra==0);
 }
 CHECK(count==77);pthread_t other;CHECK(!pthread_create(&other,NULL,foreign,NULL)&&!pthread_join(other,NULL));
 int16_t input[2048];for(unsigned i=0;i<1024;i++){input[2*i]=8192;input[2*i+1]=16384;}
 NgpVoicePcmV1 pcm={1,sizeof pcm,NGP_VOICE_PCM_S16_V1,2,48000,0,input,sizeof input,1024};
 CHECK(!ngp_voice_service_v1_bind_pcm(service,handles[0],&pcm,&v));CHECK(v.bound&&v.source_frames==1024);memset(input,0,sizeof input);
 CHECK(!ngp_voice_service_v1_enable(service,&v));CHECK(!ngp_voice_service_v1_play(service,handles[0],&v));render();
 for(unsigned frame=32;frame<256;frame++){CHECK(isfinite(output[2*frame])&&fabsf(output[2*frame]-.25f)<.0001f);CHECK(isfinite(output[2*frame+1])&&fabsf(output[2*frame+1]-.5f)<.0001f);}
 CHECK(!ngp_voice_service_v1_snapshot(service,handles[0],&v));CHECK(!v.render_allocator_violations&&v.render_sequence==1);
 uint32_t old=tokens[0][0];CHECK(!ngp_voice_tokens_v1_release(pools[0],old,&r));uint64_t native=0;CHECK(ngp_voice_tokens_v1_resolve(pools[0],old,&native,&r)==NGP_TOKENS_TOKEN&&native==0);
 uint32_t fresh=0;CHECK(!ngp_voice_tokens_v1_assign(pools[0],&fresh,&r)&&fresh!=old);CHECK(!ngp_voice_tokens_v1_resolve(pools[0],fresh,&native,&r)&&native!=handles[0]);
 for(unsigned i=0;i<3;i++){CHECK(!ngp_voice_tokens_v1_close(pools[i],&r));CHECK(r.phase==NGP_TOKENS_CLOSED&&!r.reserved_tracked&&!r.loans_tracked);CHECK(!ngp_voice_tokens_v1_close(pools[i],&r));}
 render();for(unsigned i=0;i<512;i++)CHECK(output[i]==0);
 CHECK(!ngp_voice_service_v1_close(service,&v));CHECK(!v.owned_pcm_bytes&&!v.render_allocator_violations&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1);
 return NULL;
}
int main(void){
 CHECK(pthread_main_np());NgpVoiceOutputV1 format={1,sizeof format,48000,2};NgpVoiceReportV1 v=voice_report();CHECK(!ngp_voice_service_v1_reserve(&format,&service,&v)&&service);
 NgpVoiceTokensReportV1 r=report();for(unsigned k=1;k<=3;k++)CHECK(!ngp_voice_tokens_v1_reserve(k,&pools[k-1],&r));
 pthread_attr_t attr;CHECK(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,8u*1024u*1024u));pthread_t t;CHECK(!pthread_create(&t,&attr,worker,NULL)&&!pthread_join(t,NULL));CHECK(!pthread_attr_destroy(&attr));
 printf("{\"status\":\"real-voice-token-integration-passed\",\"checks\":%u,\"cohorts\":3,\"actualVoiceLoans\":77,\"verifiedPCMFrames\":224,\"renderCallbacks\":2,\"workerStackBytes\":%llu,\"staleAndForeignRefused\":true,\"actualBackend\":true,\"physicalDeviceStarted\":false,\"iPhoneExecuted\":false}\n",checks,(unsigned long long)worker_stack);return 0;
}
