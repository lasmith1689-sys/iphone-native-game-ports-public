/* OWN direct baseline API composition; no production encoded-bind helper. */
#include "ngp_memory_mp3_v1.h"
#include "ngp_voice_service_v1.h"
#include "ngp_device_mixer_render_v1.h"
#include "fixture_cases.h"
#include <pthread.h>
#include <stdatomic.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define N 4096u
#define PI 3.14159265358979323846
#define CHECK(x) do{if(!(x)){fprintf(stderr,"own MP3 STREAM assertion line %u: %s\n",(unsigned)__LINE__,#x);_Exit(91);}}while(0)
typedef struct {double amp,rms,mean,wrong,off;} Metrics;
static int analyse(const float*l,const float*r,unsigned source_channels,Metrics out[2]){
 for(unsigned ch=0;ch<2;ch++){
  const float*p=ch?r:l;double wanted=source_channels==1||ch==0?440:880;double hz[3]={wanted,wanted==440?880:440,1320},s[3]={0},c[3]={0},sum=0,sq=0;
  for(unsigned i=0;i<N;i++){double v=p[i];if(!isfinite(v))return 0;sum+=v;sq+=v*v;for(unsigned k=0;k<3;k++){double a=2*PI*hz[k]*i/48000;s[k]+=v*sin(a);c[k]+=v*cos(a);}}
  Metrics m={2*hypot(s[0],c[0])/N,sqrt(sq/N),sum/N,2*hypot(s[1],c[1])/N,2*hypot(s[2],c[2])/N};out[ch]=m;
  if(m.amp<.30||m.amp>.60||m.rms<.20||m.rms>.46||fabs(m.mean)>.025||m.wrong>.08||m.off>.06)return 0;
 }
 return 1;
}
typedef struct {
 pthread_mutex_t lock;pthread_cond_t change;const Case*c;unsigned char*encoded;
 NgpVoiceServiceV1*service;NgpDeviceMixerRenderV1 helper;
 unsigned request,complete,done,callback_parked;float left[N],right[N];Metrics metrics[2];double energy[2];
 unsigned mp3_release_calls,mp3_null,encoded_retired,voice_release_calls,kind;
 uint64_t bound_frames,pre,post,sequence,allocator,handle;
} Fixture;
static Fixture f={.lock=PTHREAD_MUTEX_INITIALIZER,.change=PTHREAD_COND_INITIALIZER};
static void lock(void){CHECK(pthread_mutex_lock(&f.lock)==0);}static void unlock(void){CHECK(pthread_mutex_unlock(&f.lock)==0);}static void signal_change(void){CHECK(pthread_cond_broadcast(&f.change)==0);}static void wait_change(void){CHECK(pthread_cond_wait(&f.change,&f.lock)==0);}
static NgpVoiceReportV1 report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof(r);return r;}
static void*callback(void*unused){
 (void)unused;CHECK(!pthread_main_np());
 for(;;){lock();while(f.request==f.complete){f.callback_parked=1;signal_change();wait_change();}f.callback_parked=0;unsigned ticket=f.request;unlock();
  float*p[2]={f.left,f.right};size_t bytes[2]={sizeof(f.left),sizeof(f.right)};uint32_t channels[2]={1,1},silent=1;
  CHECK(ngp_device_mixer_render_v1(&f.helper,p,bytes,channels,2,N,&silent)==0);
  for(unsigned ch=0;ch<2;ch++){double energy=0;for(unsigned i=0;i<N;i++){CHECK(isfinite(p[ch][i])&&p[ch][i]==f.helper.scratch[(size_t)i*2+ch]);if(i>=N-512)energy+=(double)p[ch][i]*p[ch][i];}f.energy[ch]=energy/512;}
  if(ticket==3)CHECK(analyse(f.left,f.right,f.c->channels,f.metrics));
  lock();f.complete=ticket;signal_change();unlock();
 }
 return NULL;
}
static void render(void){lock();CHECK(f.request==f.complete&&f.request<12);f.request++;signal_change();while(f.request!=f.complete)wait_change();unlock();}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());NgpVoiceReportV1 r=report();CHECK(ngp_voice_service_v1_prepare(f.service,&r)==0);
 NgpVoiceHandleV1 h=0;CHECK(ngp_voice_service_v1_acquire(f.service,NGP_VOICE_STREAM_V1,&h,&r)==0&&r.kind==3&&r.slot_index>=64&&r.slot_index<77);f.kind=r.kind;f.handle=h;
 CHECK(f.c->bytes<=32768&&f.c->frames*f.c->channels*2u<=120000);
 NgpMemoryMp3V1*owned=NULL;CHECK(ngp_memory_mp3_v1_decode(f.encoded,f.c->bytes,&owned)==0&&owned);NgpMemoryMp3ViewV1 v;
 CHECK(ngp_memory_mp3_v1_view(owned,&v)==0&&v.pcm&&v.envelope.source_bytes==f.c->bytes&&v.envelope.pcm_frames==f.c->frames&&v.envelope.rate==f.c->rate&&v.envelope.channels==f.c->channels&&v.source_end_reached==1&&v.bitstream_integrity_verified==0&&v.gapless_applied==0);
 NgpVoicePcmV1 pcm={1,sizeof(pcm),NGP_VOICE_PCM_S16_V1,v.envelope.channels,v.envelope.rate,0,v.pcm,v.envelope.pcm_bytes,v.envelope.pcm_frames};
 CHECK(pcm.byte_count==pcm.frame_count*pcm.channels*2&&ngp_voice_service_v1_bind_pcm(f.service,h,&pcm,&r)==0&&r.kind==3&&r.bound&&r.source_frames==f.c->frames);f.bound_frames=r.source_frames;
 memset(f.encoded,0xa5,f.c->bytes);free(f.encoded);f.encoded=NULL;f.encoded_retired=1;
 f.mp3_release_calls++;CHECK(ngp_memory_mp3_v1_release(&owned)==0&&owned==NULL);f.mp3_null=1;
 CHECK(f.encoded_retired&&f.mp3_null&&ngp_voice_service_v1_enable(f.service,&r)==0&&ngp_voice_service_v1_set_looping(f.service,h,1,&r)==0&&ngp_voice_service_v1_play(f.service,h,&r)==0);
 for(unsigned i=0;i<6;i++)render();CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==0&&r.playing&&r.looping&&r.source_cursor<=f.c->frames);f.pre=r.source_cursor;
 for(unsigned i=0;i<4;i++)render();CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==0&&r.playing&&r.looping&&r.source_cursor<=f.c->frames);f.post=r.source_cursor;CHECK(f.post<f.pre&&f.energy[0]>1e-5&&f.energy[1]>1e-5);
 f.sequence=r.render_sequence;f.allocator=r.render_allocator_violations;CHECK(f.sequence>=10&&f.allocator==0);
 CHECK(ngp_voice_service_v1_stop(f.service,h,&r)==0);render();render();CHECK(f.energy[0]<1e-12&&f.energy[1]<1e-12);
 f.voice_release_calls++;CHECK(ngp_voice_service_v1_release(f.service,h,&r)==0);CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==NGP_VOICE_HANDLE_V1);
 lock();f.done=1;signal_change();for(;;)wait_change(); /* Retain actual owner until process exit. */
 return NULL;
}
static int nonce(const char*p){if(strlen(p)!=32)return 0;for(unsigned i=0;i<32;i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return 0;return 1;}
int main(int argc,char**argv){
 CHECK(pthread_main_np());
 if(argc==3&&strcmp(argv[1],"zero-oracle")==0){CHECK(nonce(argv[2]));static const float zero[N]={0};Metrics m[2];if(!analyse(zero,zero,1,m)){fprintf(stderr,"own STREAM zero oracle refused\n");return 91;}return 0;}
 CHECK(argc==4&&nonce(argv[3]));for(unsigned i=0;i<3;i++)if(strcmp(cases[i].name,argv[1])==0)f.c=&cases[i];CHECK(f.c&&f.c->bytes<=32768&&f.c->frames*f.c->channels*2u<=120000);
 FILE*file=fopen(argv[2],"rb");CHECK(file);f.encoded=malloc(f.c->bytes);CHECK(f.encoded&&fread(f.encoded,1,f.c->bytes,file)==f.c->bytes&&fgetc(file)==EOF&&!ferror(file)&&fclose(file)==0);
 NgpVoiceOutputV1 output={1,sizeof(output),48000,2};NgpVoiceReportV1 r=report();CHECK(ngp_voice_service_v1_reserve(&output,&f.service,&r)==0);f.helper.service=f.service;f.helper.channels=2;atomic_init(&f.helper.busy,0);atomic_init(&f.helper.failed,0);
 pthread_t a,b;CHECK(pthread_create(&a,NULL,callback,NULL)==0&&pthread_create(&b,NULL,worker,NULL)==0);lock();while(!f.done||!f.callback_parked)wait_change();unlock();
 printf("{\"format\":\"own-MP3-STREAM-v1\",\"nonce\":\"%s\",\"case\":\"%s\",\"sourceRate\":%u,\"sourceChannels\":%u,\"sourceFrames\":%u,\"outputRate\":48000,\"outputChannels\":2,\"kind\":%u,\"handle\":%llu,\"boundFrames\":%llu,\"mp3ReleaseCalls\":%u,\"mp3ResultNull\":%s,\"encodedRetiredBeforePlay\":%s,\"voiceReleaseCalls\":%u,\"callbackRequests\":%u,\"cursorPre\":%llu,\"cursorPost\":%llu,\"renderSequence\":%llu,\"allocatorViolations\":%llu,\"stopTailEnergy\":[%.9g,%.9g],\"amplitude\":[%.9g,%.9g],\"rms\":[%.9g,%.9g],\"mean\":[%.9g,%.9g],\"wrong\":[%.9g,%.9g],\"off\":[%.9g,%.9g],\"workersParked\":true,\"deviceGuestMilesIncrementalGaplessIntegrityProof\":false}\n",argv[3],f.c->name,f.c->rate,f.c->channels,f.c->frames,f.kind,(unsigned long long)f.handle,(unsigned long long)f.bound_frames,f.mp3_release_calls,f.mp3_null?"true":"false",f.encoded_retired?"true":"false",f.voice_release_calls,f.complete,(unsigned long long)f.pre,(unsigned long long)f.post,(unsigned long long)f.sequence,(unsigned long long)f.allocator,f.energy[0],f.energy[1],f.metrics[0].amp,f.metrics[1].amp,f.metrics[0].rms,f.metrics[1].rms,f.metrics[0].mean,f.metrics[1].mean,f.metrics[0].wrong,f.metrics[1].wrong,f.metrics[0].off,f.metrics[1].off);
 CHECK(fflush(stdout)==0);return 0;
}
