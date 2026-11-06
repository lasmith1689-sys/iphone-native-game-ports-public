/* OWN actual encoded-helper composition. No guest/Miles/retail calls. */
#include "ngp_memory_mp3_v1.h"
#include "ngp_encoded_voice_bind_v1.h"
#include "ngp_voice_gain_balance_v2.h"
#include "ngp_voice_service_v1.h"
#include "ngp_device_mixer_render_v1.h"
#include "fixture_cases.h"
#include "ngp_audio_catalog_v1.h"
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
 unsigned char*negative;NgpEncodedVoiceBindReportV1 bind_report;unsigned main_admit,foreign_admit,negative_mp3,negative_wave;double constant_mean[2];
 unsigned request,complete,done,callback_parked;float left[N],right[N];Metrics metrics[2];double energy[2];
 unsigned mp3_release_calls,mp3_null,encoded_retired,voice_release_calls,kind;
 unsigned mode;size_t input_bytes;
 NgpAudioWorkerRuntimeV1*runtime;NgpAudioCatalogV1*catalog;unsigned requests,queries;uint32_t catalog_token;
 uint64_t bound_frames,pre,post,sequence,allocator,handle;
} Fixture;
static Fixture f={.lock=PTHREAD_MUTEX_INITIALIZER,.change=PTHREAD_COND_INITIALIZER};
static void lock(void){CHECK(pthread_mutex_lock(&f.lock)==0);}static void unlock(void){CHECK(pthread_mutex_unlock(&f.lock)==0);}static void signal_change(void){CHECK(pthread_cond_broadcast(&f.change)==0);}static void wait_change(void){CHECK(pthread_cond_wait(&f.change,&f.lock)==0);}
static NgpVoiceReportV1 report(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof(r);return r;}
static NgpEncodedVoiceBindReportV1 encoded_report(void){NgpEncodedVoiceBindReportV1 r={0};r.version=1;r.size=sizeof(r);return r;}
static int constant_oracle(const float*l,const float*r,unsigned channels,double out[2]){
 const double expected8[2]={.379411876,.284558907},expected16[2]={.09375,.140625};
 for(unsigned ch=0;ch<2;ch++){const float*p=ch?r:l;double total=0;for(unsigned i=N-512;i<N;i++){if(!isfinite(p[i]))return 0;total+=p[i];}out[ch]=total/512;if(fabs(out[ch]-(channels==1?expected8[ch]:expected16[ch]))>.000003)return 0;}return 1;
}
/* ONLY broker admission/terminal is synthetic; voice/catalog/runtime/clock real. */
int ngp_audio_worker_broker_query_v1(void*b,NgpDeviceMixerFactsV1*out){CHECK(b==&f.service);*out=(NgpDeviceMixerFactsV1){1,sizeof *out,NGP_DEVICE_BROKER_ACTIVE_V1,48000,2,4096,7,f.service};return 0;}
int ngp_audio_worker_broker_wait_admit_v1(void*b,NgpVoiceServiceV1*s,uint64_t e){CHECK(b==&f.service&&s==f.service&&e==7);return 0;}
int ngp_audio_worker_broker_request_stop_v1(void*b){CHECK(b==&f.service&&++f.requests==1);NgpVoiceReportV1 r=report();CHECK(ngp_voice_service_v1_snapshot(f.service,0,&r)==NGP_VOICE_CLOSED_V1&&r.phase==NGP_VOICE_CLOSED_RETAINED_V1);CHECK(!ngp_voice_service_v1_invalidate(f.service,NGP_VOICE_CANCELLED_V1));return 0;}
int ngp_audio_worker_broker_query_stop_v1(void*b,NgpDeviceMixerStopObservationV1*out){CHECK(b==&f.service);f.queries++;*out=(NgpDeviceMixerStopObservationV1){1,sizeof *out,f.queries>=2?NGP_DEVICE_STOP_ACK_V1:NGP_DEVICE_STOP_PENDING_V1,0,7,f.service};return 0;}
static void overlap_controls(NgpVoiceHandleV1 h){
 NgpEncodedVoiceBindReportV1 r=encoded_report(),before=r;NgpMp3PaddingV1 p={17,19,23,29},pb=p;
 CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&r,NULL)==NGP_ENCODED_ARGUMENT_V1&&!memcmp(&r,&before,sizeof r));
 CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&r,(NgpMp3PaddingV1*)(uintptr_t)(UINTPTR_MAX-7))==NGP_ENCODED_ARGUMENT_V1&&!memcmp(&r,&before,sizeof r));
 CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&r,(NgpMp3PaddingV1*)((unsigned char*)&r+64))==NGP_ENCODED_ARGUMENT_V1&&!memcmp(&r,&before,sizeof r));
 unsigned char prefix[32];memcpy(prefix,f.encoded,sizeof prefix);
 CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&r,(NgpMp3PaddingV1*)f.encoded)==NGP_ENCODED_ARGUMENT_V1&&!memcmp(&r,&before,sizeof r)&&!memcmp(prefix,f.encoded,sizeof prefix));
 union{max_align_t a;NgpEncodedVoiceBindReportV1 report;unsigned char b[1024];}u;memset(&u,0xa5,sizeof u);NgpEncodedVoiceBindReportV1*alias=&u.report;*alias=encoded_report();unsigned char copy[1024];memcpy(copy,u.b,sizeof copy);
 CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,u.b,sizeof u.b,alias,&p)==NGP_ENCODED_ARGUMENT_V1&&!memcmp(copy,u.b,sizeof copy)&&!memcmp(&p,&pb,sizeof p));
 r=encoded_report();r.version=9;before=r;CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&r,&p)==NGP_ENCODED_ARGUMENT_V1&&!memcmp(&r,&before,sizeof r)&&!memcmp(&p,&pb,sizeof p));
}
static void*foreign(void*unused){(void)unused;NgpEncodedVoiceBindReportV1 r=encoded_report();CHECK(ngp_encoded_voice_bind_v1(f.service,1,f.encoded,f.c->bytes,f.c->kind,&r)==NGP_ENCODED_VOICE_FAILED_V1&&r.admission_status==NGP_VOICE_OWNER_V1&&r.decode_status==NGP_ENCODED_NOT_RUN_V1&&r.wave_status==NGP_ENCODED_NOT_RUN_V1);f.foreign_admit=(unsigned)r.admission_status;NgpMp3PaddingV1 p={17,19,23,29},pb=p;r=encoded_report();CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,1,f.encoded,f.input_bytes,&r,&p)==NGP_ENCODED_VOICE_FAILED_V1&&r.admission_status==NGP_VOICE_OWNER_V1&&r.decode_status==NGP_ENCODED_NOT_RUN_V1&&!memcmp(&p,&pb,sizeof p));return NULL;}
static void*callback(void*unused){
 (void)unused;CHECK(!pthread_main_np());
 for(;;){lock();while(f.request==f.complete){f.callback_parked=1;signal_change();wait_change();}f.callback_parked=0;unsigned ticket=f.request;unlock();
  float*p[2]={f.left,f.right};size_t bytes[2]={sizeof(f.left),sizeof(f.right)};uint32_t channels[2]={1,1},silent=1;
  CHECK(ngp_device_mixer_render_v1(&f.helper,p,bytes,channels,2,N,&silent)==0);
  for(unsigned ch=0;ch<2;ch++){double energy=0;for(unsigned i=0;i<N;i++){CHECK(isfinite(p[ch][i])&&p[ch][i]==f.helper.scratch[(size_t)i*2+ch]);if(i>=N-512)energy+=(double)p[ch][i]*p[ch][i];}f.energy[ch]=energy/512;}
  if(ticket==3){if(f.c->kind==2)CHECK(analyse(f.left,f.right,f.c->channels,f.metrics));else CHECK(constant_oracle(f.left,f.right,f.c->channels,f.constant_mean));}
  lock();f.complete=ticket;signal_change();unlock();
 }
 return NULL;
}
static void render(void){lock();CHECK(f.request==f.complete&&f.request<12);f.request++;signal_change();while(f.request!=f.complete)wait_change();unlock();}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np());NgpVoiceReportV1 r=report();NgpAudioWorkerReportV1 w={0};w.version=1;w.size=sizeof w;
 CHECK(!ngp_audio_worker_v1_open(&f.service,&f.runtime,&w));NgpAudioWorkerContextsV1 contexts={0};CHECK(!ngp_audio_worker_v1_borrow(f.runtime,&contexts,&w)&&contexts.service==f.service);
 NgpAudioCatalogReportV1 cr={0};cr.version=1;cr.size=sizeof cr;CHECK(!ngp_audio_catalog_v1_bind(f.runtime,&f.catalog,&cr));
 uint32_t provider_legacy=UINT32_MAX;CHECK(!ngp_audio_catalog_v1_open_provider(f.catalog,4,&provider_legacy,&cr)&&provider_legacy==0&&cr.phase==NGP_CATALOG_OPEN);
 NgpVoiceHandleV1 h=0;CHECK(!ngp_audio_catalog_v1_acquire_ordinary(f.catalog,&f.catalog_token,&cr)&&f.catalog_token);CHECK(!ngp_audio_catalog_v1_resolve_voice(f.catalog,f.catalog_token,&h,&cr)&&h);CHECK(!ngp_voice_service_v1_snapshot(f.service,h,&r)&&r.kind==1&&r.slot_index<32);f.kind=r.kind;f.handle=h;
 overlap_controls(h);
 CHECK(f.c->bytes<=131072&&f.c->frames*f.c->channels*2u<=120000);
 float pitch=f.c->kind==1?.5f:1,gain=f.c->kind==1?.75f:1,balance=f.c->kind==1?-.25f:0;
 CHECK(ngp_voice_service_v1_set_pitch(f.service,h,pitch,&r)==0&&ngp_voice_service_v1_set_looping(f.service,h,1,&r)==0);
 NgpVoiceGainBalanceReportV2 pair={0};pair.version=2;pair.size=sizeof(pair);CHECK(ngp_voice_service_v2_set_gain_balance(f.service,h,gain,balance,&pair)==0);
 if(f.mode){NgpEncodedVoiceBindReportV1 strict=encoded_report();CHECK(ngp_encoded_voice_bind_v1(f.service,h,f.encoded,f.input_bytes,NGP_ENCODED_MP3_V1,&strict)==NGP_ENCODED_FILE_REFUSED_V1&&strict.decode_status==NGP_MP3_INVALID_V1&&!strict.copy_committed&&!strict.helper_poisoned);}
 f.bind_report=encoded_report();NgpMp3PaddingV1 padding={0xa5a5a5a5u,0xa5a5a5a5u,0xa5a5a5a5u,0xa5a5a5a5u};
 int padded_code=ngp_encoded_voice_bind_padded_mp3_v1(f.service,h,f.encoded,f.input_bytes,&f.bind_report,&padding);
 CHECK(padded_code==0&&padding.source_bytes==f.input_bytes&&padding.padding_bytes==(f.mode?7u:0u)&&padding.suffix_bytes==f.c->bytes&&padding.kind==f.mode);
 CHECK(f.bind_report.encoded_bytes==f.input_bytes&&f.bind_report.mp3.source_bytes==f.c->bytes);
 NgpEncodedVoiceBindReportV1*q=&f.bind_report;CHECK(q->admission_status==0&&q->bind_status==0&&q->copy_committed==1&&q->helper_poisoned==0&&q->voice_after.source_frames==f.c->frames&&q->voice_after.looping==1&&q->voice_after.requested_pitch==pitch&&q->voice_after.requested_volume==gain&&!q->voice_after.playing);
 if(f.c->kind==2)CHECK(q->decode_status==0&&q->view_status==0&&q->release_status==0&&q->release_confirmed==1&&q->wave_status==NGP_ENCODED_NOT_RUN_V1);else CHECK(q->wave_status==0&&q->decode_status==NGP_ENCODED_NOT_RUN_V1&&q->release_status==NGP_ENCODED_NOT_RUN_V1&&!q->release_attempted);
 r=q->voice_after;f.bound_frames=r.source_frames;f.mp3_release_calls=q->release_attempted;f.mp3_null=q->release_confirmed;
 NgpEncodedVoiceBindReportV1 no=encoded_report();CHECK(ngp_encoded_voice_bind_v1(f.service,h,f.negative,14976,NGP_ENCODED_MP3_V1,&no)==NGP_ENCODED_FILE_REFUSED_V1&&no.decode_status==NGP_MP3_UNSUPPORTED_V1&&no.bind_status==NGP_ENCODED_NOT_RUN_V1&&!no.copy_committed&&!no.helper_poisoned);f.negative_mp3=(unsigned)no.decode_status;
 const unsigned char bad[12]={'R','I','F','F',64,0,0,0,'W','A','V','E'};no=encoded_report();CHECK(ngp_encoded_voice_bind_v1(f.service,h,bad,sizeof(bad),NGP_ENCODED_WAVE_PCM_V1,&no)==NGP_ENCODED_FILE_REFUSED_V1&&no.wave_status==NGP_WAVE_INVALID_V1&&no.bind_status==NGP_ENCODED_NOT_RUN_V1&&!no.copy_committed&&!no.helper_poisoned);f.negative_wave=(unsigned)no.wave_status;
 CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==0&&r.bound&&r.source_frames==f.c->frames&&r.looping==1&&r.requested_pitch==pitch&&r.requested_volume==gain);
 memset(f.encoded,0xa5,f.input_bytes);free(f.encoded);f.encoded=NULL;free(f.negative);f.negative=NULL;f.encoded_retired=1;
 CHECK(f.encoded_retired&&ngp_voice_service_v1_enable(f.service,&r)==0&&ngp_voice_service_v1_set_looping(f.service,h,1,&r)==0&&ngp_voice_service_v1_play(f.service,h,&r)==0);
 for(unsigned i=0;i<6;i++)render();CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==0&&r.playing&&r.looping&&r.source_cursor<=f.c->frames);f.pre=r.source_cursor;
 for(unsigned i=0;i<4;i++)render();CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==0&&r.playing&&r.looping&&r.source_cursor<=f.c->frames);f.post=r.source_cursor;CHECK(f.post<f.pre&&f.energy[0]>1e-5&&f.energy[1]>1e-5);
 f.sequence=r.render_sequence;f.allocator=r.render_allocator_violations;CHECK(f.sequence>=10&&f.allocator==0);
 CHECK(ngp_voice_service_v1_stop(f.service,h,&r)==0);render();render();CHECK(f.energy[0]<1e-12&&f.energy[1]<1e-12);
 f.voice_release_calls++;CHECK(ngp_audio_catalog_v1_release_voice(f.catalog,f.catalog_token,&cr)==0);CHECK(ngp_voice_service_v1_snapshot(f.service,h,&r)==NGP_VOICE_HANDLE_V1);
 lock();while(!f.callback_parked)wait_change();unlock();
 NgpAudioWorkerStopReportV1 stop={0};stop.version=1;stop.size=sizeof stop;CHECK(!ngp_audio_worker_v1_wait_stop(f.runtime,&stop)&&stop.close_ack&&stop.phase==NGP_AUDIO_WORKER_STOPPED_RETAINED&&!stop.first_worker_status&&f.requests==1&&f.queries==2);
 lock();f.done=1;signal_change();for(;;)wait_change(); /* Retain actual owner until process exit. */
 return NULL;
}
static int nonce(const char*p){if(strlen(p)!=32)return 0;for(unsigned i=0;i<32;i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return 0;return 1;}
int main(int argc,char**argv){
 CHECK(pthread_main_np());
 if(argc==3&&strcmp(argv[1],"zero-oracle")==0){CHECK(nonce(argv[2]));static const float zero[N]={0};Metrics m[2];double means[2];if(!analyse(zero,zero,1,m)&&!constant_oracle(zero,zero,1,means)&&!constant_oracle(zero,zero,2,means)){fprintf(stderr,"own STREAM zero oracle refused\n");return 91;}return 0;}
 CHECK(argc==6&&nonce(argv[4])&&strlen(argv[5])==1&&argv[5][0]>='0'&&argv[5][0]<='2');f.mode=(unsigned)(argv[5][0]-'0');for(unsigned i=0;i<5;i++)if(strcmp(cases[i].name,argv[1])==0)f.c=&cases[i];CHECK(f.c&&f.c->bytes<=131072&&f.c->frames*f.c->channels*2u<=120000);
 FILE*file=fopen(argv[2],"rb");CHECK(file);f.encoded=malloc(f.c->bytes);CHECK(f.encoded&&fread(f.encoded,1,f.c->bytes,file)==f.c->bytes&&fgetc(file)==EOF&&!ferror(file)&&fclose(file)==0);
 f.input_bytes=f.c->bytes+(f.mode?7u:0u);if(f.mode){unsigned char*p=calloc(1,f.input_bytes);CHECK(p);if(f.mode==2)p[0]=0xff;memcpy(p+7,f.encoded,f.c->bytes);free(f.encoded);f.encoded=p;}
 FILE*negative=fopen(argv[3],"rb");CHECK(negative);f.negative=malloc(14976);CHECK(f.negative&&fread(f.negative,1,14976,negative)==14976&&fgetc(negative)==EOF&&!ferror(negative)&&fclose(negative)==0);
 NgpVoiceOutputV1 output={1,sizeof(output),48000,2};NgpVoiceReportV1 r=report();CHECK(ngp_voice_service_v1_reserve(&output,&f.service,&r)==0);f.helper.service=f.service;f.helper.channels=2;atomic_init(&f.helper.busy,0);atomic_init(&f.helper.failed,0);
 NgpEncodedVoiceBindReportV1 main_report=encoded_report();CHECK(ngp_encoded_voice_bind_v1(f.service,1,f.encoded,f.c->bytes,f.c->kind,&main_report)==NGP_ENCODED_VOICE_FAILED_V1&&main_report.admission_status==NGP_VOICE_OWNER_V1&&main_report.decode_status==NGP_ENCODED_NOT_RUN_V1&&main_report.wave_status==NGP_ENCODED_NOT_RUN_V1);f.main_admit=(unsigned)main_report.admission_status;NgpMp3PaddingV1 main_pad={17,19,23,29},mp=main_pad;main_report=encoded_report();CHECK(ngp_encoded_voice_bind_padded_mp3_v1(f.service,1,f.encoded,f.input_bytes,&main_report,&main_pad)==NGP_ENCODED_VOICE_FAILED_V1&&main_report.admission_status==NGP_VOICE_OWNER_V1&&main_report.decode_status==NGP_ENCODED_NOT_RUN_V1&&!memcmp(&main_pad,&mp,sizeof mp));
 pthread_t foreign_thread;CHECK(pthread_create(&foreign_thread,NULL,foreign,NULL)==0&&pthread_join(foreign_thread,NULL)==0);
 pthread_attr_t attr;CHECK(pthread_attr_init(&attr)==0&&pthread_attr_setstacksize(&attr,8u*1024u*1024u)==0);pthread_t a,b;CHECK(pthread_create(&a,NULL,callback,NULL)==0&&pthread_create(&b,&attr,worker,NULL)==0&&pthread_attr_destroy(&attr)==0);lock();while(!f.done||!f.callback_parked)wait_change();unlock();
 printf("{\"format\":\"own-PADDED-ENCODED-CATALOG-v1\",\"nonce\":\"%s\",\"case\":\"%s\",\"sourceRate\":%u,\"sourceChannels\":%u,\"sourceFrames\":%u,\"outputRate\":48000,\"outputChannels\":2,\"kind\":%u,\"handle\":%llu,\"boundFrames\":%llu,\"mp3ReleaseCalls\":%u,\"mp3ResultNull\":%s,\"encodedRetiredBeforePlay\":%s,\"voiceReleaseCalls\":%u,\"callbackRequests\":%u,\"cursorPre\":%llu,\"cursorPost\":%llu,\"renderSequence\":%llu,\"allocatorViolations\":%llu,\"stopTailEnergy\":[%.9g,%.9g],\"amplitude\":[%.9g,%.9g],\"rms\":[%.9g,%.9g],\"mean\":[%.9g,%.9g],\"wrong\":[%.9g,%.9g],\"off\":[%.9g,%.9g],\"workersParked\":true,\"deviceGuestMilesIncrementalGaplessIntegrityProof\":false,\"formatKind\":%u,\"constantMean\":[%.9g,%.9g],\"admission\":%d,\"wave\":%d,\"decode\":%d,\"view\":%d,\"bind\":%d,\"release\":%d,\"copyCommitted\":%u,\"releaseConfirmed\":%u,\"helperPoisoned\":%u,\"negativeMP3\":%u,\"negativeWAVE\":%u,\"mainAdmission\":%u,\"foreignAdmission\":%u,\"helperReportBytes\":%u}\n",argv[4],f.c->name,f.c->rate,f.c->channels,f.c->frames,f.kind,(unsigned long long)f.handle,(unsigned long long)f.bound_frames,f.mp3_release_calls,f.mp3_null?"true":"false",f.encoded_retired?"true":"false",f.voice_release_calls,f.complete,(unsigned long long)f.pre,(unsigned long long)f.post,(unsigned long long)f.sequence,(unsigned long long)f.allocator,f.energy[0],f.energy[1],f.metrics[0].amp,f.metrics[1].amp,f.metrics[0].rms,f.metrics[1].rms,f.metrics[0].mean,f.metrics[1].mean,f.metrics[0].wrong,f.metrics[1].wrong,f.metrics[0].off,f.metrics[1].off,f.c->kind,f.constant_mean[0],f.constant_mean[1],f.bind_report.admission_status,f.bind_report.wave_status,f.bind_report.decode_status,f.bind_report.view_status,f.bind_report.bind_status,f.bind_report.release_status,f.bind_report.copy_committed,f.bind_report.release_confirmed,f.bind_report.helper_poisoned,f.negative_mp3,f.negative_wave,f.main_admit,f.foreign_admit,(unsigned)sizeof(NgpEncodedVoiceBindReportV1));
 printf("{\"status\":\"actual-catalogue-padded-copy-render-wait-passed\",\"mode\":%u,\"requests\":%u,\"queries\":%u,\"paddingBytes\":%u,\"originalBytes\":%llu,\"suffixBytes\":%u,\"overlapControls\":6,\"realVoiceCatalogueDecoderWait\":true,\"brokerSynthetic\":true,\"deviceGameProof\":false}\n",f.mode,f.requests,f.queries,f.mode?7:0,(unsigned long long)f.input_bytes,f.c->bytes);CHECK(fflush(stdout)==0);return 0;
}
