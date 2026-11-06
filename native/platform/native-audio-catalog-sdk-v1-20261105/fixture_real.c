/* Real software backend/catalog on Mac; broker facts explicitly SYNTHETIC.
 * Actual UIKit broker/bridge compile for iPhone separately, never run here. */
#include "ngp_audio_catalog_v1.h"
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NgpVoiceServiceV1*service;static NgpAudioWorkerRuntimeV1*runtime;static NgpAudioCatalogV1*catalog;
static NgpAudioWorkerContextsV1 contexts;static uint64_t handles[77],worker_stack;
static uint32_t tokens[3][32];static float output[512];static int render_code;
static unsigned checks,queries,callbacks,stop_requests;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"real audio catalog assertion%u line%d\n",checks,__LINE__);exit(4);}}while(0)
static NgpAudioWorkerReportV1 wr(void){NgpAudioWorkerReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpAudioCatalogReportV1 cr(void){NgpAudioCatalogReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 tr(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
int ngp_audio_worker_broker_query_v1(void*b,NgpDeviceMixerFactsV1*f){
 CHECK(b==&service&&f->version==1&&f->size==sizeof *f);queries++;
 *f=(NgpDeviceMixerFactsV1){1,sizeof *f,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,service};return 0;
}
int ngp_audio_worker_broker_request_stop_v1(void*b){CHECK(b==&service);stop_requests++;return 0;}
static void*callback(void*unused){(void)unused;render_code=ngp_voice_service_v1_render(service,output,256,256);return NULL;}
static void render(void){pthread_t t;CHECK(!pthread_create(&t,NULL,callback,NULL)&&!pthread_join(t,NULL));CHECK(!render_code);callbacks++;}
static void*foreign(void*unused){
 (void)unused;NgpAudioCatalogReportV1 r=cr();uint32_t result=0x55aa;unsigned before=queries;
 CHECK(ngp_audio_catalog_v1_open_provider(catalog,4,&result,&r)==NGP_CATALOG_OWNER&&!r.worker_observation&&result==0x55aa);CHECK(queries==before);return NULL;
}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);
 NgpAudioWorkerReportV1 w=wr();CHECK(!ngp_audio_worker_v1_open(&service,&runtime,&w));
 CHECK(runtime&&w.phase==NGP_AUDIO_WORKER_READY&&w.ordinary_reserved==32&&w.positional_reserved==32&&w.stream_reserved==13&&w.voice_prepared&&w.gate_enabled&&!stop_requests);
 CHECK(!ngp_audio_worker_v1_borrow(runtime,&contexts,&w));CHECK(contexts.service==service&&contexts.ordinary&&contexts.positional&&contexts.streams);
 NgpAudioCatalogReportV1 c=cr();CHECK(!ngp_audio_catalog_v1_bind(runtime,&catalog,&c)&&catalog&&c.phase==NGP_CATALOG_BOUND);
 char name[64];memset(name,0x5a,sizeof name);uint32_t it=0,provider=0;
 CHECK(ngp_audio_catalog_v1_enumerate(catalog,&it,&provider,name,1,&c)==NGP_CATALOG_ARGUMENT&&!it&&!provider&&name[0]==0x5a);
 CHECK(!ngp_audio_catalog_v1_enumerate(catalog,&it,&provider,name,sizeof name,&c)&&it==1&&provider==4&&!strcmp(name,"Native iPhone Stereo Positional"));
 char copy[64];memcpy(copy,name,sizeof name);CHECK(!ngp_audio_catalog_v1_enumerate(catalog,&it,&provider,name,sizeof name,&c)&&!it&&!provider&&!memcmp(name,copy,sizeof name));
 uint32_t legacy=0x55aa;CHECK(ngp_audio_catalog_v1_open_provider(catalog,5,&legacy,&c)==NGP_CATALOG_TOKEN&&legacy==0x55aa);
 CHECK(!ngp_audio_catalog_v1_open_provider(catalog,4,&legacy,&c)&&!legacy&&c.phase==NGP_CATALOG_OPEN);
 uint32_t attr=0;CHECK(!ngp_audio_catalog_v1_attribute(catalog,4,"Maximum supported samples",25,&attr,&c)&&attr==32);
 CHECK(!ngp_audio_catalog_v1_attribute(catalog,4,"EAX3 room LF",12,&attr,&c)&&attr==UINT32_MAX);
 CHECK(ngp_audio_catalog_v1_set_distance_factor(catalog,4,0,&c)==NGP_CATALOG_ARGUMENT);
 CHECK(!ngp_audio_catalog_v1_set_distance_factor(catalog,4,.0254f,&c)&&fabsf(c.metres_per_unit-.0254f)<.000001f);
 unsigned count=0;NgpVoiceTokensReportV1 t=tr();
 for(unsigned kind=1;kind<=3;kind++){
  unsigned cap=kind==3?13:32;
  for(unsigned i=0;i<cap;i++){
   uint64_t h=0;
   if(kind==3){CHECK(!ngp_voice_tokens_v1_assign(contexts.streams,&tokens[2][i],&t));CHECK(!ngp_voice_tokens_v1_resolve(contexts.streams,tokens[2][i],&h,&t));}
   else{CHECK(!(kind==1?ngp_audio_catalog_v1_acquire_ordinary(catalog,&tokens[0][i],&c):ngp_audio_catalog_v1_acquire_positional(catalog,4,&tokens[1][i],&c)));CHECK(!ngp_audio_catalog_v1_resolve_voice(catalog,tokens[kind-1][i],&h,&c));}
   CHECK(h&&tokens[kind-1][i]&&(tokens[kind-1][i]&3u)==kind);
   for(unsigned j=0;j<count;j++)CHECK(h!=handles[j]);handles[count++]=h;
  }
  uint32_t extra=0;
  if(kind==3)CHECK(ngp_voice_tokens_v1_assign(contexts.streams,&extra,&t)==NGP_TOKENS_LIMIT&&!extra);
  else CHECK((kind==1?ngp_audio_catalog_v1_acquire_ordinary(catalog,&extra,&c):ngp_audio_catalog_v1_acquire_positional(catalog,4,&extra,&c))==NGP_CATALOG_LIMIT&&!extra);
 }
 CHECK(count==77&&c.ordinary_loans_tracked==32&&c.positional_loans_tracked==32);
 pthread_t other;CHECK(!pthread_create(&other,NULL,foreign,NULL)&&!pthread_join(other,NULL));
 NgpVoiceReportV1 v=vr();int16_t pcm[2048];for(unsigned i=0;i<1024;i++){pcm[i*2]=8192;pcm[i*2+1]=16384;}
 NgpVoicePcmV1 clip={1,sizeof clip,NGP_VOICE_PCM_S16_V1,2,48000,0,pcm,sizeof pcm,1024};
 CHECK(!ngp_voice_service_v1_bind_pcm(service,handles[0],&clip,&v));memset(pcm,0,sizeof pcm);CHECK(!ngp_voice_service_v1_play(service,handles[0],&v));render();
 for(unsigned i=32;i<256;i++){CHECK(isfinite(output[i*2])&&fabsf(output[i*2]-.25f)<.0001f);CHECK(isfinite(output[i*2+1])&&fabsf(output[i*2+1]-.5f)<.0001f);}
 CHECK(!ngp_voice_service_v1_snapshot(service,handles[0],&v)&&!v.render_allocator_violations&&v.render_sequence==1);
 CHECK(!ngp_audio_catalog_v1_close_provider(catalog,4,&c)&&c.phase==NGP_CATALOG_CLOSED&&!c.positional_loans_tracked);
 for(unsigned i=0;i<32;i++)CHECK(!ngp_audio_catalog_v1_release_voice(catalog,tokens[0][i],&c));CHECK(!c.ordinary_loans_tracked);
 /* Catalog is not used after this controlled final driver/service teardown. */
 CHECK(!ngp_voice_tokens_v1_close(contexts.ordinary,&t));CHECK(!ngp_voice_tokens_v1_close(contexts.streams,&t));
 render();for(unsigned i=0;i<512;i++)CHECK(output[i]==0);
 CHECK(!ngp_voice_service_v1_close(service,&v)&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1&&!v.owned_pcm_bytes&&!v.render_allocator_violations);return NULL;
}
int main(void){
 CHECK(pthread_main_np());NgpVoiceOutputV1 f={1,sizeof f,48000,2};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_reserve(&f,&service,&v)&&service);
 pthread_attr_t a;CHECK(!pthread_attr_init(&a)&&!pthread_attr_setstacksize(&a,8388608));pthread_t t;CHECK(!pthread_create(&t,&a,worker,NULL)&&!pthread_join(t,NULL));CHECK(!pthread_attr_destroy(&a));
 printf("{\"status\":\"real-audio-catalog-join-passed\",\"checks\":%u,\"actualVoiceLoans\":77,\"cohorts\":3,\"verifiedPCMFrames\":224,\"renderCallbacks\":%u,\"workerStackBytes\":%llu,\"foreignRefused\":true,\"actualCatalogBound\":true,\"actualVoiceBackend\":true,\"brokerIsSynthetic\":true,\"physicalDeviceStarted\":false,\"iPhoneExecuted\":false}\n",checks,callbacks,(unsigned long long)worker_stack);return 0;
}
