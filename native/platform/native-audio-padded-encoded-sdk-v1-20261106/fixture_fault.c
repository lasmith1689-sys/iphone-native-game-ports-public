/* OWN actual backend/codec + explicitly injected typed boundary faults. */
#include "test_fault.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static unsigned done;static _Atomic unsigned checks;static NgpEncodedVoiceBindReportV1 final;
static void check(int b,int line){atomic_fetch_add_explicit(&checks,1,memory_order_relaxed);if(!b){fprintf(stderr,"{\"status\":\"encoded-fault-assertion-failed\",\"line\":%d,\"mode\":%u}\n",line,test_mode);_Exit(101);}}
#define CHECK(x)check(!!(x),__LINE__)
static NgpEncodedVoiceBindReportV1 er(void){NgpEncodedVoiceBindReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int entry(int padded,NgpEncodedVoiceBindReportV1*r,NgpMp3PaddingV1*p){return padded?ngp_encoded_voice_bind_padded_mp3_v1(test_service,test_handle,test_original,test_original_bytes,r,p):ngp_encoded_voice_bind_v1(test_service,test_handle,test_original,test_original_bytes,NGP_ENCODED_MP3_V1,r);}
static void*worker(void*unused){
 (void)unused;CHECK(!pthread_main_np()&&pthread_get_stacksize_np(pthread_self())>=8388608);NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_prepare(test_service,&v)&&test_sound_inits==77);CHECK(!ngp_voice_service_v1_acquire(test_service,NGP_VOICE_STREAM_V1,&test_handle,&v));
 int padded=test_mode!=0&&test_mode!=2;NgpMp3PaddingV1 p={17,19,23,29},before=p;final=er();int code=entry(padded,&final,&p);
 if(test_mode==2||test_mode==3){
  CHECK(code==0&&test_nested==1&&test_decodes==1&&test_views==1&&test_releases==1&&final.copy_committed&&final.release_confirmed&&!final.helper_poisoned);
  NgpEncodedVoiceBindReportV1 again=er();p=before;const void*original=test_original;size_t original_bytes=test_original_bytes;
  if(padded){test_original=(const unsigned char*)original+7;test_original_bytes-=7;} /* old strict entry still receives ONLY its unprefixed suffix */
  CHECK(entry(!padded,&again,&p)==0&&test_decodes==2&&test_releases==2&&!again.helper_poisoned);test_original=original;test_original_bytes=original_bytes; /* actual subsequent cross-entry admission, busy cleared */
 }else{
  CHECK(code==NGP_ENCODED_POISONED_V1&&final.status==NGP_ENCODED_POISONED_V1&&final.helper_poisoned&&final.guard_state_observed&&test_decodes==1&&!memcmp(&p,&before,sizeof p));
  unsigned origin=test_mode==4?NGP_ENCODED_ORIGIN_DECODE_V1:test_mode==5?NGP_ENCODED_ORIGIN_VIEW_V1:test_mode==6?NGP_ENCODED_ORIGIN_PADDING_V1:test_mode==7?NGP_ENCODED_ORIGIN_BIND_V1:NGP_ENCODED_ORIGIN_RELEASE_V1;
  CHECK(final.first_error_origin==origin);
  if(test_mode==4)CHECK(!test_views&&!test_releases&&!final.decoder_created&&final.decode_status==NGP_MP3_CODEC_V1&&final.decoder_quarantined_identity&&final.decoder_retention_uncertain);
  else CHECK(final.decode_status==0&&final.decoder_created&&test_releases==1&&final.release_attempted);
  if(test_mode==6)CHECK(!test_views&&final.view_status==NGP_ENCODED_NOT_RUN_V1&&final.release_confirmed&&!final.copy_committed);
  if(test_mode==5)CHECK(test_views==1&&final.view_status==NGP_MP3_CODEC_V1&&final.release_confirmed&&!final.copy_committed);
  if(test_mode==0||test_mode==1)CHECK(final.copy_committed&&!final.release_confirmed&&final.decoder_quarantined_identity&&final.decoder_retention_uncertain);
  if(test_mode==7)CHECK(test_sound_inits==78&&final.bind_status==NGP_VOICE_LIBRARY_V1&&!final.copy_committed&&final.voice_after.owned_pcm_bytes==110592&&final.voice_after.phase==NGP_VOICE_FAILED_RETAINED_V1&&final.release_status==NGP_MP3_CODEC_V1);
  unsigned d=test_decodes,q=test_views,z=test_releases;NgpEncodedVoiceBindReportV1 again=er();p=before;
  CHECK(entry(!padded,&again,&p)==NGP_ENCODED_POISONED_V1&&again.first_error_origin==NGP_ENCODED_ORIGIN_GUARD_V1&&again.admission_status==NGP_ENCODED_NOT_RUN_V1&&test_decodes==d&&test_views==q&&test_releases==z&&!memcmp(&p,&before,sizeof p));
 }
 CHECK(!pthread_mutex_lock(&lock));done=1;CHECK(!pthread_cond_signal(&changed));for(;;)CHECK(!pthread_cond_wait(&changed,&lock));return NULL;
}
int main(int argc,char**argv){
 CHECK(pthread_main_np()&&argc==3&&strlen(argv[1])==1&&argv[1][0]>='0'&&argv[1][0]<='7');test_mode=(unsigned)(argv[1][0]-'0');FILE*f=fopen(argv[2],"rb");CHECK(f);size_t extra=test_mode!=0&&test_mode!=2?7:0;unsigned char*p=calloc(1,15046+extra);CHECK(p&&fread(p+extra,1,15046,f)==15046&&fgetc(f)==EOF&&!ferror(f)&&!fclose(f));test_original=p;test_original_bytes=15046+extra;
 NgpVoiceOutputV1 output={1,sizeof output,48000,2};NgpVoiceReportV1 v=vr();CHECK(!ngp_voice_service_v1_reserve(&output,&test_service,&v));pthread_attr_t a;CHECK(!pthread_attr_init(&a)&&!pthread_attr_setstacksize(&a,8388608));pthread_t t;CHECK(!pthread_create(&t,&a,worker,NULL)&&!pthread_attr_destroy(&a));CHECK(!pthread_mutex_lock(&lock));while(!done)CHECK(!pthread_cond_wait(&changed,&lock));CHECK(!pthread_mutex_unlock(&lock));
 printf("{\"status\":\"actual-encoded-typed-fault-passed\",\"mode\":%u,\"checks\":%u,\"decodes\":%u,\"views\":%u,\"releases\":%u,\"nested\":%u,\"soundInits\":%u,\"firstOrigin\":%u,\"copy\":%u,\"poison\":%u,\"reportBytes\":%u,\"testOnlyInterceptions\":true,\"realCodecVoiceLinked\":true,\"deviceGameProof\":false}\n",test_mode,atomic_load(&checks),test_decodes,test_views,test_releases,test_nested,test_sound_inits,final.first_error_origin,final.copy_committed,final.helper_poisoned,(unsigned)sizeof final);CHECK(!fflush(stdout));return 0;
}
