/* Own real miniaudio fixture; no device, guest or retail input. */
#include "ngp_positional_provider_v1.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NgpVoiceServiceV1*service;static NgpPositionalProviderV1*provider;
static uint32_t tokens[32];static uint64_t handles[32],worker_stack;
static float output[1024];static unsigned checks,callbacks;static int render_code;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"real positional assertion%u line%d\n",checks,__LINE__);exit(4);}}while(0)
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpPositionalReportV1 pr(void){NgpPositionalReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static void*callback(void*unused){(void)unused;render_code=ngp_voice_service_v1_render(service,output,512,512);return NULL;}
static void render(void){pthread_t t;CHECK(!pthread_create(&t,NULL,callback,NULL)&&!pthread_join(t,NULL));CHECK(!render_code);callbacks++;}
/* Pinned miniaudio defaults: inverse distance, minDistance1, rolloff1. This
 * backend's engine defaults to8ms smoothing (384frames at48000Hz).
 * Three512-frame callbacks settle every transition.
 * The fixed front direction keeps panning constant, so power ratios derive
 * solely from inverse distance. These ratios precede any native outputs. */
static double settled_power(void){for(int i=0;i<3;i++)render();double sum=0;for(unsigned i=768;i<1024;i++){CHECK(isfinite(output[i])&&output[i]>0&&output[i]<=.251f);sum+=(double)output[i]*output[i];}return sum/256;}
static void*foreign(void*unused){(void)unused;NgpPositionalReportV1 r=pr();uint64_t h=0;CHECK(ngp_positional_v1_resolve(provider,tokens[0],&h,&r)==NGP_POSITIONAL_OWNER&&!h&&!r.worker_observation);CHECK(ngp_positional_v1_set_factor(provider,2,&r)==NGP_POSITIONAL_OWNER&&!r.worker_observation);CHECK(ngp_positional_v1_close(provider,&r)==NGP_POSITIONAL_OWNER&&!r.worker_observation);return NULL;}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());worker_stack=pthread_get_stacksize_np(pthread_self());CHECK(worker_stack>=8388608);NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_prepare(service,&v));NgpPositionalReportV1 r=pr();CHECK(!ngp_positional_v1_open(service,&provider,&r)&&provider&&r.phase==NGP_POSITIONAL_LIVE&&r.capacity==32);
 for(unsigned i=0;i<32;i++){CHECK(!ngp_positional_v1_acquire(provider,&tokens[i],&r));CHECK(!ngp_positional_v1_resolve(provider,tokens[i],&handles[i],&r)&&handles[i]);for(unsigned j=0;j<i;j++)CHECK(handles[i]!=handles[j]&&tokens[i]!=tokens[j]);}
 uint32_t extra=0;CHECK(ngp_positional_v1_acquire(provider,&extra,&r)==NGP_POSITIONAL_LIMIT&&!extra&&r.active==32);
 pthread_t t;CHECK(!pthread_create(&t,NULL,foreign,NULL)&&!pthread_join(t,NULL));
 int16_t input[4096];for(unsigned i=0;i<4096;i++)input[i]=8192;NgpVoicePcmV1 pcm={1,sizeof pcm,NGP_VOICE_PCM_S16_V1,1,48000,0,input,sizeof input,4096};
 CHECK(!ngp_voice_service_v1_bind_pcm(service,handles[0],&pcm,&v));memset(input,0,sizeof input);CHECK(!ngp_voice_service_v1_set_looping(service,handles[0],1,&v));CHECK(!ngp_positional_v1_set_position(provider,tokens[0],0,0,-1,&r));CHECK(!ngp_positional_v1_set_listener(provider,0,0,0,&r));CHECK(!ngp_voice_service_v1_enable(service,&v)&&!ngp_voice_service_v1_play(service,handles[0],&v));
 double near=settled_power();CHECK(near>.0001&&near<.1);
 CHECK(!ngp_positional_v1_set_factor(provider,10,&r));CHECK(!ngp_voice_service_v1_snapshot(service,handles[0],&v)&&v.position[2]==-10);double far=settled_power();CHECK(fabs(far/near-.01)<.0003);
 CHECK(!ngp_positional_v1_set_factor(provider,2,&r));CHECK(!ngp_positional_v1_set_listener(provider,0,0,-.5f,&r));double shifted=settled_power();CHECK(fabs(shifted/near-1)<.03);
 CHECK(!ngp_positional_v1_set_factor(provider,4,&r));CHECK(!ngp_voice_service_v1_snapshot(service,handles[0],&v)&&v.position[2]==-4);double rescaled=settled_power();CHECK(fabs(rescaled/near-.25)<.0075);
 CHECK(ngp_positional_v1_set_factor(provider,NAN,&r)==NGP_POSITIONAL_ARGUMENT&&r.phase==NGP_POSITIONAL_LIVE&&r.metres_per_unit==4);CHECK(ngp_positional_v1_set_factor(provider,2000000,&r)==NGP_POSITIONAL_ARGUMENT);CHECK(!ngp_voice_service_v1_snapshot(service,handles[0],&v)&&v.position[2]==-4&&!v.render_allocator_violations);double unchanged=settled_power();CHECK(fabs(unchanged/rescaled-1)<.03);
 uint32_t old=tokens[0];uint64_t oldhandle=handles[0];CHECK(!ngp_positional_v1_release(provider,old,&r));uint64_t h=0;CHECK(ngp_positional_v1_resolve(provider,old,&h,&r)==NGP_POSITIONAL_TOKEN&&!h);uint32_t fresh=0;CHECK(!ngp_positional_v1_acquire(provider,&fresh,&r)&&fresh!=old);CHECK(!ngp_positional_v1_resolve(provider,fresh,&h,&r)&&h!=oldhandle);
 CHECK(!ngp_positional_v1_close(provider,&r)&&r.phase==NGP_POSITIONAL_CLOSED&&!r.active&&!r.capacity);CHECK(!ngp_positional_v1_close(provider,&r));render();for(unsigned i=0;i<1024;i++)CHECK(output[i]==0);
 CHECK(!ngp_voice_service_v1_close(service,&v)&&!v.owned_pcm_bytes&&!v.render_allocator_violations&&v.phase==NGP_VOICE_CLOSED_RETAINED_V1);
 printf("{\"status\":\"real-positional-provider-passed\",\"checks\":%u,\"actualVoiceLoans\":32,\"renderCallbacks\":%u,\"settledWindows\":5,\"workerStackBytes\":%llu,\"inverseDistancePowerRatio\":%.12g,\"listenerShiftPowerRatio\":%.12g,\"rescaledPowerRatio\":%.12g,\"staleAndForeignRefused\":true,\"actualBackend\":true,\"physicalDeviceStarted\":false,\"iPhoneExecuted\":false}\n",checks,callbacks,(unsigned long long)worker_stack,far/near,shifted/near,rescaled/near);return NULL;
}
int main(void){CHECK(pthread_main_np());NgpVoiceOutputV1 format={1,sizeof format,48000,2};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_reserve(&format,&service,&v)&&service);pthread_attr_t a;CHECK(!pthread_attr_init(&a)&&!pthread_attr_setstacksize(&a,8388608));pthread_t t;CHECK(!pthread_create(&t,&a,worker,NULL)&&!pthread_join(t,NULL));CHECK(!pthread_attr_destroy(&a));return 0;}
