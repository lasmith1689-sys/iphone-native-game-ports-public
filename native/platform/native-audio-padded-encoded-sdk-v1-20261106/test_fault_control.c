/* OWN explicit TEST ONLY faults. Real codec runs; never compiled into product. */
#include "test_fault.h"
#include "ngp_audio_shared_profile_v1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned test_mode,test_decodes,test_views,test_releases,test_nested,test_sound_inits;
NgpVoiceServiceV1*test_service;NgpVoiceHandleV1 test_handle;
const void*test_original;size_t test_original_bytes;
static void need(int ok){if(!ok){fprintf(stderr,"{\"status\":\"typed-test-interception-failed\"}\n");_Exit(102);}}
static NgpEncodedVoiceBindReportV1 report(void){NgpEncodedVoiceBindReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
int ngp_memory_mp3_v1_decode(const void*p,size_t n,NgpMemoryMp3V1**out){
 test_decodes++;
 if((test_mode==2||test_mode==3)&&!test_nested){
  test_nested++;NgpEncodedVoiceBindReportV1 r=report();NgpMp3PaddingV1 pad={17,19,23,29},before=pad;
  int rc=test_mode==2?ngp_encoded_voice_bind_padded_mp3_v1(test_service,test_handle,test_original,test_original_bytes,&r,&pad):ngp_encoded_voice_bind_v1(test_service,test_handle,test_original,test_original_bytes,NGP_ENCODED_MP3_V1,&r);
  need(rc==NGP_ENCODED_BUSY_V1&&r.first_error_origin==NGP_ENCODED_ORIGIN_GUARD_V1&&r.guard_state_observed&&!r.helper_poisoned&&r.admission_status==NGP_ENCODED_NOT_RUN_V1&&r.decode_status==NGP_ENCODED_NOT_RUN_V1&&!memcmp(&pad,&before,sizeof pad));
 }
 int code=actual_memory_decode(p,n,out);
 if(test_mode==4&&!code){need(*out!=NULL);return NGP_MP3_CODEC_V1;} /* authentic object retained, never released after contradictory status */
 return code;
}
int ngp_memory_mp3_v1_view(const NgpMemoryMp3V1*p,NgpMemoryMp3ViewV1*out){test_views++;int c=actual_memory_view(p,out);return test_mode==5&&!c?NGP_MP3_CODEC_V1:c;}
int ngp_memory_mp3_v1_release(NgpMemoryMp3V1**p){
 test_releases++;NgpMemoryMp3V1*identity=*p;int c=actual_memory_release(p);need(c==0&&!*p);
 if(test_mode==1){*p=identity;return 0;} /* deliberately stale identity: test helper never dereferences it */
 return test_mode==0||test_mode==7?NGP_MP3_CODEC_V1:c;
}
int ngp_mp3_zero_prefix_v1_decode(const void*p,size_t n,NgpMemoryMp3V1**out,NgpMp3PaddingV1*padding){
 int code=actual_prefix_decode(p,n,out,padding);if(test_mode==6&&!code)padding->source_bytes--;return code;
}
ma_result test_sound_init(ma_engine*e,ma_data_source*d,ma_uint32 flags,ma_sound_group*g,ma_sound*s){
 test_sound_inits++;
 if(test_mode==7&&test_sound_inits==78)return MA_ERROR; /* actual pending PCM memcpy already completed, no fabricated commit */
 return ma_sound_init_from_data_source(e,d,flags,g,s);
}
