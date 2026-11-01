/* OWN synthetic fixture. Real service/shared implementation; no retail/Apple
 * device/guest execution. Main reserves; one persistent worker controls the
 * graph; another persistent pthread executes every render request. */
#include "ngp_voice_service_v1.h"
#include "ngp_voice_gain_balance_v2.h"
#include "ngp_device_mixer_render_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define N 16384u
#define R 4096u
#define CHECK(x) do{if(!(x)){fprintf(stderr,"own fixture assertion line %u: %s\n",(unsigned)__LINE__,#x);_Exit(80);}}while(0)
typedef struct {
 pthread_mutex_t lock;pthread_cond_t changed;
 NgpVoiceServiceV1 *service;NgpDeviceMixerRenderV1 helper;
 uint32_t rate,channels,request,complete,mode,ready,start,done;
 int code;uint32_t silent;double mean,energy;uint64_t callback_requests;
 float interleaved[R*2],left[R],right[R];
 uint32_t acquired[3];uint64_t last_token,reacquired,slow_delta,fast_delta;
 double copy_mean,u8_mean,s16_mean,near_energy,far_energy;
 double channel_mean[2],gain_means[20];uint32_t gain_cases,gain_mono_code;
 NgpVoiceReportV1 before,after,terminal;
} Fixture;
static Fixture f={.lock=PTHREAD_MUTEX_INITIALIZER,.changed=PTHREAD_COND_INITIALIZER};
static float pcm[N*2],positional[N];static uint8_t u8[N];static int16_t s16[N*2];
static NgpVoiceReportV1 report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof(r);return r;}
static void lock(void){CHECK(pthread_mutex_lock(&f.lock)==0);}
static void unlock(void){CHECK(pthread_mutex_unlock(&f.lock)==0);}
static void signal_change(void){CHECK(pthread_cond_broadcast(&f.changed)==0);}
static void wait_change(void){CHECK(pthread_cond_wait(&f.changed,&f.lock)==0);}
static int all_zero(const float*p,size_t n){for(size_t i=0;i<n;i++)if(p[i]!=0)return 0;return 1;}
static void *callback_thread(void*unused){
 (void)unused;CHECK(!pthread_main_np());
 for(;;){
  lock();while(f.request==f.complete)wait_change();uint32_t ticket=f.request,mode=f.mode;unlock();
  CHECK(ticket!=0);int code;uint32_t silent=99;double sum=0,energy=0,channel_sum[2]={0,0};
  for(size_t i=0;i<R*2;i++)f.interleaved[i]=123;
  for(size_t i=0;i<R;i++)f.left[i]=f.right[i]=123;
  if(mode==3)CHECK(ngp_voice_service_v1_invalidate(f.service,NGP_VOICE_ROUTE_V1)==0);
  if(mode==2){
   float*planes[2]={f.left,f.right};size_t bytes[2]={sizeof(f.left),sizeof(f.right)};uint32_t channels[2]={1,1};
   code=ngp_device_mixer_render_v1(&f.helper,planes,bytes,channels,f.channels,R,&silent);
   if(code==0&&!silent)for(uint32_t ch=0;ch<f.channels;ch++)for(uint32_t i=0;i<R;i++)CHECK(planes[ch][i]==f.helper.scratch[(size_t)i*f.channels+ch]);
   for(uint32_t ch=0;ch<f.channels;ch++)for(uint32_t i=R-512;i<R;i++){float x=planes[ch][i];CHECK(isfinite(x));sum+=x;channel_sum[ch]+=x;energy+=(double)x*x;}
   if(code||silent){CHECK(all_zero(f.left,R));if(f.channels==2)CHECK(all_zero(f.right,R));}
  }else{
   code=ngp_voice_service_v1_render(f.service,f.interleaved,R,R);silent=code?1:0;
   for(uint32_t i=(R-512)*f.channels;i<R*f.channels;i++){float x=f.interleaved[i];CHECK(isfinite(x));sum+=x;channel_sum[i%f.channels]+=x;energy+=(double)x*x;}
   if(code)CHECK(all_zero(f.interleaved,R*f.channels));
  }
  lock();f.code=code;f.silent=silent;f.mean=sum/(512*f.channels);f.energy=energy/(512*f.channels);f.callback_requests++;for(uint32_t ch=0;ch<f.channels;ch++)f.channel_mean[ch]=channel_sum[ch]/512;f.complete=ticket;signal_change();unlock();
 }
 return NULL;
}
/* A fixture handshake, never a service runtime hook. Calls are serialized and
 * buffers are immutable to the worker until callback completion is observed. */
static int render(uint32_t mode){
 lock();CHECK(f.request==f.complete&&f.request<UINT32_MAX);f.mode=mode;f.request++;signal_change();while(f.request!=f.complete)wait_change();int code=f.code;unlock();return code;
}
static void steady(uint32_t mode){CHECK(render(mode)==0);CHECK(render(mode)==0);}
static NgpVoicePcmV1 description(const void*p,uint32_t format,uint32_t channels,uint32_t rate){
 uint64_t unit=format==1?1:format==2?2:4;
 NgpVoicePcmV1 d={1,sizeof(d),format,channels,rate,0,p,N*channels*unit,N};return d;
}
static uint64_t delta(uint64_t a,uint64_t b){CHECK(a<=N&&b<=N);a%=N;b%=N;return b>=a?b-a:N-a+b;}
/* Inserted OWN fixture section. No service/runtime testing hook. */
const uint32_t ngp_voice_gain_report_bytes_fixture_v2=sizeof(NgpVoiceGainBalanceReportV2);
const uint32_t ngp_voice_gain_inner_offset_fixture_v2=offsetof(NgpVoiceGainBalanceReportV2,voice);
const uint32_t ngp_voice_gain_inner_bytes_fixture_v2=sizeof(NgpVoiceReportV1);
const uint32_t ngp_voice_gain_balance_offset_fixture_v2=offsetof(NgpVoiceGainBalanceReportV2,requested_native_balance);
const uint32_t ngp_voice_gain_valid_offset_fixture_v2=offsetof(NgpVoiceGainBalanceReportV2,gain_observation_valid);
static NgpVoiceGainBalanceReportV2 gain_report(void){NgpVoiceGainBalanceReportV2 q={0};q.version=2;q.size=sizeof(q);return q;}
static void gain_check(double left,double right){
 steady(2);CHECK(f.gain_cases<10);CHECK(fabs(f.channel_mean[0]-left)<0.000001&&fabs(f.channel_mean[1]-right)<0.000001);
 f.gain_means[f.gain_cases*2]=f.channel_mean[0];f.gain_means[f.gain_cases*2+1]=f.channel_mean[1];f.gain_cases++;
}
static void exercise_gain(NgpVoiceHandleV1 ordinary,NgpVoiceHandleV1 positional_handle,NgpVoiceHandleV1 stream){
 NgpVoiceReportV1 r=report();NgpVoiceGainBalanceReportV2 q=gain_report();
 CHECK(q.voice.version==0); /* Embedded input header is intentionally absent. */
 CHECK(ngp_voice_service_v2_set_gain_balance(f.service,positional_handle,.5f,.5f,&q)==NGP_VOICE_KIND_V1);
 CHECK(ngp_voice_service_v1_set_volume(f.service,ordinary,1.5f,&r)==NGP_VOICE_ARGUMENT_V1);
 if(f.channels==1){
  CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,.5f,.5f,&q)==NGP_VOICE_UNSUPPORTED_V2&&q.voice.phase==NGP_VOICE_READY_OPEN_V1&&q.voice.requested_volume==1&&q.gain_observation_valid==1);f.gain_mono_code=NGP_VOICE_UNSUPPORTED_V2;return;
 }
 CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,17,0,&q)==NGP_VOICE_ARGUMENT_V1);CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,1,2,&q)==NGP_VOICE_ARGUMENT_V1);
 for(unsigned i=0;i<N;i++){pcm[2*i]=0.125f;pcm[2*i+1]=0.25f;}
 NgpVoicePcmV1 d=description(pcm,NGP_VOICE_PCM_F32_V1,2,f.rate);
 CHECK(ngp_voice_service_v1_bind_pcm(f.service,ordinary,&d,&r)==0);CHECK(ngp_voice_service_v1_set_looping(f.service,ordinary,1,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,ordinary,&r)==0);
 /* Independent literal output goldens for independent L/R source channels. */
 const float pairs[6][4]={{.5f,0,.0625f,.125f},{.5f,-1,.0625f,0},{.5f,1,0,.125f},{.75f,-.25f,.09375f,.140625f},{0,.8f,0,0},{1.5f,0,.1875f,.375f}};
 for(unsigned i=0;i<6;i++){
  CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,pairs[i][0],pairs[i][1],&q)==0&&q.voice.version==1&&q.voice.size==sizeof(q.voice)&&q.gain_observation_valid==1&&q.requested_linear_gain==pairs[i][0]&&q.requested_native_balance==pairs[i][1]&&q.output_channels==2&&q.balance_mode==0);
  CHECK(ngp_voice_service_v1_snapshot(f.service,ordinary,&r)==0&&r.requested_volume==pairs[i][0]);gain_check(pairs[i][2],pairs[i][3]);
 }
 CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,.5f,.5f,&q)==0);CHECK(ngp_voice_service_v1_set_volume(f.service,ordinary,.75f,&r)==0);gain_check(.046875,.1875); /* V1 preserves balance. */
 CHECK(ngp_voice_service_v2_set_gain_balance(f.service,ordinary,.75f,-.25f,&q)==0);
 for(unsigned i=0;i<N;i++){pcm[2*i]=.25f;pcm[2*i+1]=.125f;}
 CHECK(ngp_voice_service_v1_bind_pcm(f.service,ordinary,&d,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,ordinary,&r)==0);gain_check(.1875,.0703125); /* Rebind preserves pair. */
 CHECK(ngp_voice_service_v1_release(f.service,ordinary,&r)==0);NgpVoiceHandleV1 fresh=0;CHECK(ngp_voice_service_v1_acquire(f.service,NGP_VOICE_ORDINARY_V1,&fresh,&r)==0&&r.requested_volume==1);
 for(unsigned i=0;i<N;i++){pcm[2*i]=.125f;pcm[2*i+1]=.25f;}
 CHECK(ngp_voice_service_v1_bind_pcm(f.service,fresh,&d,&r)==0);CHECK(ngp_voice_service_v1_set_looping(f.service,fresh,1,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,fresh,&r)==0);gain_check(.125,.25); /* Release/reset cleared balance. */
 CHECK(ngp_voice_service_v1_stop(f.service,fresh,&r)==0);
 CHECK(ngp_voice_service_v1_bind_pcm(f.service,stream,&d,&r)==0);CHECK(ngp_voice_service_v1_set_looping(f.service,stream,1,&r)==0);CHECK(ngp_voice_service_v2_set_gain_balance(f.service,stream,.5f,.5f,&q)==0);CHECK(ngp_voice_service_v1_play(f.service,stream,&r)==0);gain_check(.03125,.125);CHECK(ngp_voice_service_v1_stop(f.service,stream,&r)==0);
 CHECK(ngp_voice_service_v1_snapshot(f.service,fresh,&r)==0&&r.render_allocator_violations==0);
}

static void *engine_worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());NgpVoiceReportV1 r=report();NgpVoiceHandleV1 handles[77]={0};
 CHECK(ngp_voice_service_v1_prepare(f.service,&r)==0&&r.phase==NGP_VOICE_READY_GATED_V1);
 lock();f.ready=1;signal_change();while(!f.start)wait_change();unlock();
 CHECK(render(1)==NGP_VOICE_GATED_V1);CHECK(ngp_voice_service_v1_enable(f.service,&r)==0);
 unsigned index=0;
 for(uint32_t kind=1;kind<=3;kind++){
  unsigned count=kind==3?13:32;
  for(unsigned i=0;i<count;i++){
   CHECK(index<77);CHECK(ngp_voice_service_v1_acquire(f.service,kind,&handles[index],&r)==0);
   CHECK(handles[index]>f.last_token&&r.kind==kind&&r.bound==0&&!r.playing);f.last_token=handles[index++];f.acquired[kind-1]++;
  }
  NgpVoiceHandleV1 extra=0;CHECK(ngp_voice_service_v1_acquire(f.service,kind,&extra,&r)==NGP_VOICE_LIMIT_V1&&extra==0&&r.phase==NGP_VOICE_READY_OPEN_V1);
 }
 CHECK(index==77);CHECK(ngp_voice_service_v1_play(f.service,handles[1],&r)==NGP_VOICE_UNBOUND_V1);
 for(unsigned i=0;i<N*2;i++)pcm[i]=0.125f;
 NgpVoicePcmV1 d=description(pcm,NGP_VOICE_PCM_F32_V1,f.channels,f.rate);
 CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[0],&d,&r)==0&&r.bound&&r.source_frames==N);
 for(unsigned i=0;i<N*2;i++)pcm[i]=0; /* Caller changes AFTER synchronous copy. */
 CHECK(ngp_voice_service_v1_set_looping(f.service,handles[0],1,&r)==0&&r.looping==1);
 CHECK(ngp_voice_service_v1_play(f.service,handles[0],&r)==0);steady(2);f.copy_mean=f.mean;CHECK(f.silent==0&&fabs(f.copy_mean-0.125)<0.005);
 for(unsigned i=0;i<N;i++)u8[i]=192;
 d=description(u8,NGP_VOICE_PCM_U8_V1,1,22050);CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[1],&d,&r)==0);
 uint64_t untouched=r.source_cursor;CHECK(untouched==0);steady(1);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[1],&r)==0&&r.source_cursor==untouched);
 CHECK(render(1)==0);CHECK(ngp_voice_service_v1_snapshot(f.service,handles[0],&r)==0&&r.playing&&r.looping==1); /* More than N output/source-rate frames read. */
 /* Pitch checks observe actual source-domain progression after one warm block;
  * they do not equate requested control time with device presentation time. */
 CHECK(ngp_voice_service_v1_set_pitch(f.service,handles[0],0.5f,&r)==0&&r.requested_pitch==0.5f);
 CHECK(ngp_voice_service_v1_seek(f.service,handles[0],0,&r)==0&&r.seek_may_be_pending==1);CHECK(render(1)==0);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[0],&r)==0);uint64_t a=r.source_cursor;CHECK(render(1)==0);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[0],&r)==0);f.slow_delta=delta(a,r.source_cursor);
 CHECK(ngp_voice_service_v1_set_pitch(f.service,handles[0],2,&r)==0&&r.requested_pitch==2);
 CHECK(ngp_voice_service_v1_seek(f.service,handles[0],0,&r)==0&&r.seek_may_be_pending==1);CHECK(render(1)==0);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[0],&r)==0);a=r.source_cursor;CHECK(render(1)==0);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[0],&r)==0);f.fast_delta=delta(a,r.source_cursor);CHECK(f.slow_delta>0&&f.fast_delta>2*f.slow_delta);
 CHECK(ngp_voice_service_v1_set_volume(f.service,handles[0],0,&r)==0&&r.requested_volume==0);steady(1);CHECK(f.energy<1e-12);
 CHECK(ngp_voice_service_v1_stop(f.service,handles[0],&r)==0&&!r.playing);steady(1);CHECK(f.energy<1e-12);
 CHECK(ngp_voice_service_v1_set_looping(f.service,handles[1],1,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,handles[1],&r)==0);steady(1);f.u8_mean=f.mean;CHECK(fabs(f.u8_mean-0.5)<0.01);
 CHECK(ngp_voice_service_v1_stop(f.service,handles[1],&r)==0);
 for(unsigned i=0;i<N*2;i++)s16[i]=8192;
 d=description(s16,NGP_VOICE_PCM_S16_V1,2,32000);CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[2],&d,&r)==0);
 CHECK(ngp_voice_service_v1_set_looping(f.service,handles[2],1,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,handles[2],&r)==0);steady(1);f.s16_mean=f.mean;CHECK(fabs(f.s16_mean-0.25)<0.01);CHECK(ngp_voice_service_v1_stop(f.service,handles[2],&r)==0);
 for(unsigned i=0;i<N;i++)positional[i]=0.25f;
 d=description(positional,NGP_VOICE_PCM_F32_V1,1,f.rate);CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[32],&d,&r)==0);
 CHECK(ngp_voice_service_v1_set_listener_position(f.service,0,0,0,&r)==0);CHECK(ngp_voice_service_v1_set_position(f.service,handles[32],0,0,-1,&r)==0);
 CHECK(ngp_voice_service_v1_set_looping(f.service,handles[32],1,&r)==0);CHECK(ngp_voice_service_v1_play(f.service,handles[32],&r)==0);steady(2);f.near_energy=f.energy;
 CHECK(ngp_voice_service_v1_set_position(f.service,handles[32],0,0,-100,&r)==0);steady(2);f.far_energy=f.energy;CHECK(f.near_energy>0&&f.far_energy<f.near_energy*0.2);CHECK(ngp_voice_service_v1_stop(f.service,handles[32],&r)==0);
 /* Complete typed precondition refusals, without allocating huge test clips. */
 d=description(pcm,NGP_VOICE_PCM_F32_V1,1,f.rate);d.byte_count--;CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[3],&d,&r)==NGP_VOICE_ARGUMENT_V1);
 d=description(pcm,NGP_VOICE_PCM_F32_V1,1,f.rate);d.channels=3;CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[3],&d,&r)==NGP_VOICE_ARGUMENT_V1);
 d=description(pcm,NGP_VOICE_PCM_F32_V1,1,f.rate);uint32_t nan=0x7fc00000u;memcpy(pcm,&nan,4);CHECK(ngp_voice_service_v1_bind_pcm(f.service,handles[3],&d,&r)==NGP_VOICE_ARGUMENT_V1);
 CHECK(ngp_voice_service_v1_set_pitch(f.service,handles[0],0,&r)==NGP_VOICE_ARGUMENT_V1);CHECK(ngp_voice_service_v1_set_position(f.service,handles[0],0,0,0,&r)==NGP_VOICE_KIND_V1);
 NgpVoiceHandleV1 old=handles[0],fresh=0;CHECK(ngp_voice_service_v1_release(f.service,old,&r)==0);
 CHECK(ngp_voice_service_v1_acquire(f.service,1,&fresh,&r)==0&&fresh>f.last_token&&r.generation==2&&r.requested_pitch==1&&r.requested_volume==1&&r.looping==0);f.reacquired=fresh;
 CHECK(ngp_voice_service_v1_play(f.service,old,&r)==NGP_VOICE_HANDLE_V1);CHECK(ngp_voice_service_v1_play(f.service,fresh,&r)==NGP_VOICE_UNBOUND_V1);
 exercise_gain(fresh,handles[32],handles[64]);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[1],&f.before)==0);
 CHECK(f.before.render_allocator_violations==0&&f.before.engine_heap_bytes>0&&f.before.owned_pcm_bytes>0);
 steady(1);CHECK(ngp_voice_service_v1_snapshot(f.service,handles[1],&f.after)==0);
 CHECK(f.after.render_sequence>f.before.render_sequence&&f.after.render_allocator_violations==0);
 CHECK(render(3)==NGP_VOICE_ROUTE_V1);CHECK(render(1)==NGP_VOICE_ROUTE_V1);
 CHECK(ngp_voice_service_v1_snapshot(f.service,handles[1],&f.terminal)==NGP_VOICE_ROUTE_V1&&f.terminal.phase==NGP_VOICE_FAILED_RETAINED_V1&&f.terminal.status==NGP_VOICE_ROUTE_V1);
 CHECK(f.terminal.owned_pcm_bytes==f.after.owned_pcm_bytes&&f.terminal.engine_heap_bytes==f.after.engine_heap_bytes&&f.terminal.render_allocator_violations==0);
 CHECK(ngp_voice_service_v1_enable(f.service,&r)==NGP_VOICE_ROUTE_V1);
 CHECK(ngp_voice_service_v1_close(f.service,&r)==NGP_VOICE_ROUTE_V1&&r.phase==NGP_VOICE_FAILED_RETAINED_V1);
 NgpVoiceGainBalanceReportV2 failed={0};failed.version=2;failed.size=sizeof(failed);CHECK(ngp_voice_service_v2_set_gain_balance(f.service,handles[1],1,0,&failed)==NGP_VOICE_ROUTE_V1&&failed.gain_observation_valid==0&&failed.voice.status==NGP_VOICE_ROUTE_V1);
 lock();f.done=1;signal_change();for(;;)wait_change(); /* Keep original worker identity live until process exit. */
 return NULL;
}
int main(int argc,char**argv){
 CHECK(pthread_main_np()&&argc==4);CHECK(strlen(argv[3])==32);for(unsigned i=0;i<32;i++)CHECK((argv[3][i]>='0'&&argv[3][i]<='9')||(argv[3][i]>='a'&&argv[3][i]<='f'));
 f.rate=(uint32_t)strtoul(argv[1],NULL,10);f.channels=(uint32_t)strtoul(argv[2],NULL,10);CHECK((f.rate==44100&&f.channels==1)||(f.rate==48000&&f.channels==2));
 f.before=report();f.after=report();f.terminal=report();NgpVoiceReportV1 r=report();NgpVoiceOutputV1 output={1,sizeof(output),96000,f.channels};
 CHECK(ngp_voice_service_v1_reserve(&output,&f.service,&r)==NGP_VOICE_ARGUMENT_V1&&f.service==NULL);output.sample_rate=f.rate;
 CHECK(ngp_voice_service_v1_reserve(&output,&f.service,&r)==0&&f.service&&r.phase==NGP_VOICE_RESERVED_V1);
 CHECK(ngp_voice_service_v1_prepare(f.service,&r)==NGP_VOICE_OWNER_V1);
 f.helper.service=f.service;f.helper.channels=f.channels;atomic_init(&f.helper.busy,0);atomic_init(&f.helper.failed,0);
 pthread_t callback,worker;CHECK(pthread_create(&callback,NULL,callback_thread,NULL)==0);
 CHECK(render(1)==NGP_VOICE_GATED_V1);CHECK(render(2)==0&&f.silent==1&&atomic_load_explicit(&f.helper.failed,memory_order_seq_cst)==0);
 CHECK(pthread_create(&worker,NULL,engine_worker,NULL)==0);lock();while(!f.ready)wait_change();unlock();
 CHECK(ngp_voice_service_v1_snapshot(f.service,1,&r)==NGP_VOICE_OWNER_V1);CHECK(ngp_voice_service_v1_enable(f.service,&r)==NGP_VOICE_OWNER_V1);
 NgpVoiceGainBalanceReportV2 foreign={0};foreign.version=2;foreign.size=sizeof(foreign);CHECK(ngp_voice_service_v2_set_gain_balance(f.service,1,1,0,&foreign)==NGP_VOICE_OWNER_V1&&foreign.gain_observation_valid==0&&foreign.voice.version==1);
 lock();f.start=1;signal_change();while(!f.done)wait_change();unlock();
 /* Neither worker exits/joins; main cannot create an expired worker identity. */
 CHECK(ngp_voice_service_v1_snapshot(f.service,1,&r)==NGP_VOICE_OWNER_V1);
 printf("{\"nonce\":\"%s\",\"rate\":%u,\"channels\":%u,\"acquired\":[%u,%u,%u],\"lastInitialHandle\":%llu,\"reacquiredHandle\":%llu,\"callbackRequests\":%llu,\"slowSourceDelta\":%llu,\"fastSourceDelta\":%llu,\"copyMean\":%.9g,\"u8Mean\":%.9g,\"s16Mean\":%.9g,\"nearEnergy\":%.9g,\"farEnergy\":%.9g,\"renderSequence\":%llu,\"allocatorViolations\":%llu,\"retainedPCMBytes\":%llu,\"retainedEngineBytes\":%llu,\"terminalStatus\":%u,\"terminalPhase\":%u,\"workerStillParked\":true,\"realService\":true,\"AppleAudioOrGameExecuted\":false,\"gainCases\":%u,\"gainMonoCode\":%u,\"gainReportBytes\":%u,\"gainMeans\":[",
 argv[3],f.rate,f.channels,f.acquired[0],f.acquired[1],f.acquired[2],(unsigned long long)f.last_token,(unsigned long long)f.reacquired,(unsigned long long)f.callback_requests,(unsigned long long)f.slow_delta,(unsigned long long)f.fast_delta,f.copy_mean,f.u8_mean,f.s16_mean,f.near_energy,f.far_energy,(unsigned long long)f.after.render_sequence,(unsigned long long)f.terminal.render_allocator_violations,(unsigned long long)f.terminal.owned_pcm_bytes,(unsigned long long)f.terminal.engine_heap_bytes,f.terminal.status,f.terminal.phase,f.gain_cases,f.gain_mono_code,(unsigned)sizeof(NgpVoiceGainBalanceReportV2));
 for(uint32_t i=0;i<f.gain_cases*2;i++){printf(i?",%.9g":"%.9g",f.gain_means[i]);}
 printf("]}\n");
 CHECK(fflush(stdout)==0);return 0;
}
