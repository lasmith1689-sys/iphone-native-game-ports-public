/* OWN synchronous worker binding, not a guest stream or filesystem backend. */
#include "ngp_encoded_voice_bind_v1.h"
#include <string.h>
#include <stdint.h>
_Static_assert(sizeof(NgpEncodedVoiceBindReportV1)<=1024,"bounded own bind report");
_Static_assert(sizeof(size_t)<=sizeof(uint64_t),"bounded native source count");
typedef struct {
 uint32_t busy,poisoned,quarantine_present,retention_uncertain;
 NgpMemoryMp3V1 *quarantined_identity;
} Guard;
/* No per-voice registry. Exactly one unexpected identity can be quarantined;
 * poison prevents another admitted call. The stored identity is NEVER read. */
static _Thread_local Guard guard;
static int span(const void*p,size_t n){return p&&n&&(uintptr_t)p<=UINTPTR_MAX-n;}
static int overlap(const void*a,size_t an,const void*b,size_t bn){return(uintptr_t)a<(uintptr_t)b+bn&&(uintptr_t)b<(uintptr_t)a+an;}
static void initialize(NgpEncodedVoiceBindReportV1*r,size_t bytes,uint32_t kind){
 memset(r,0,sizeof(*r));r->version=1;r->size=sizeof(*r);r->status=NGP_ENCODED_ARGUMENT_V1;r->format_kind=kind;r->encoded_bytes=bytes;
 r->admission_status=NGP_ENCODED_NOT_RUN_V1;r->wave_status=NGP_ENCODED_NOT_RUN_V1;r->decode_status=NGP_ENCODED_NOT_RUN_V1;
 r->view_status=NGP_ENCODED_NOT_RUN_V1;r->bind_status=NGP_ENCODED_NOT_RUN_V1;r->release_status=NGP_ENCODED_NOT_RUN_V1;
 r->voice_before.version=1;r->voice_before.size=sizeof(r->voice_before);r->voice_before.status=UINT32_MAX;
 r->voice_after.version=1;r->voice_after.size=sizeof(r->voice_after);r->voice_after.status=UINT32_MAX;
}
static void first_error(NgpEncodedVoiceBindReportV1*r,uint32_t origin){if(r->first_error_origin==NGP_ENCODED_ORIGIN_NONE_V1)r->first_error_origin=origin;}
static int argument(NgpEncodedVoiceBindReportV1*r,int code){first_error(r,NGP_ENCODED_ORIGIN_ARGUMENT_V1);r->status=(uint32_t)code;return code;}
static int complete(NgpEncodedVoiceBindReportV1*r,int code,int release_busy){
 r->status=(uint32_t)code;r->guard_state_observed=1;r->helper_poisoned=guard.poisoned;r->decoder_quarantined_identity=guard.quarantine_present;r->decoder_retention_uncertain=guard.retention_uncertain;
 if(release_busy)guard.busy=0;return code;
}
static void poison(NgpEncodedVoiceBindReportV1*r,NgpMemoryMp3V1*identity,uint32_t uncertain){
 guard.poisoned=1;if(identity){guard.quarantined_identity=identity;guard.quarantine_present=1;}
 if(uncertain)guard.retention_uncertain=1;
 r->helper_poisoned=1;r->decoder_quarantined_identity=guard.quarantine_present;r->decoder_retention_uncertain=guard.retention_uncertain;
}
static void release_decoder(NgpMemoryMp3V1**owned,NgpEncodedVoiceBindReportV1*r){
 /* This local identity was produced by the authenticated same-worker decode.
  * It is an identity only if a contradictory cleanup outcome occurs. */
 NgpMemoryMp3V1*identity=*owned;r->release_attempted=1;int code=ngp_memory_mp3_v1_release(owned);r->release_status=code;
 if(code==0&&*owned==NULL){r->release_confirmed=1;return;}
 first_error(r,NGP_ENCODED_ORIGIN_RELEASE_V1);poison(r,identity,(uint32_t)(code==0||*owned==NULL));
}
static int bind_wave(NgpVoiceServiceV1*service,NgpVoiceHandleV1 token,const void*source,size_t bytes,NgpEncodedVoiceBindReportV1*r){
 NgpWavePcmV1 info={0};int code=ngp_wave_pcm_v1_parse(source,bytes,&info);r->wave_status=code;
 if(code){first_error(r,NGP_ENCODED_ORIGIN_WAVE_V1);return NGP_ENCODED_FILE_REFUSED_V1;}
 r->wave=info;r->wave_info_valid=1;
 uint64_t unit=info.bits==8?1:info.bits==16?2:0;
 /* Parent semantics: bounded declared RIFF container may ignore trailing bytes. */
 if(info.format_tag!=1||!unit||info.channels<1||info.channels>2||!info.frames||!info.data_bytes||info.sample_rate<8000||info.sample_rate>192000||
    info.container_bytes>bytes||info.data_offset>info.container_bytes||info.data_bytes>info.container_bytes-info.data_offset||
    (uint64_t)info.frames*info.channels*unit!=info.data_bytes||info.block_align!=info.channels*unit||(uint64_t)info.sample_units*unit!=info.data_bytes){
  first_error(r,NGP_ENCODED_ORIGIN_WAVE_V1);return NGP_ENCODED_FILE_REFUSED_V1;
 }
 NgpVoicePcmV1 pcm={1,sizeof(pcm),info.bits==8?NGP_VOICE_PCM_U8_V1:NGP_VOICE_PCM_S16_V1,info.channels,info.sample_rate,0,(const uint8_t*)source+info.data_offset,info.data_bytes,info.frames};
 code=ngp_voice_service_v1_bind_pcm(service,token,&pcm,&r->voice_after);r->bind_status=code;r->copy_committed=code==0;
 if(code){first_error(r,NGP_ENCODED_ORIGIN_BIND_V1);return NGP_ENCODED_VOICE_FAILED_V1;}return NGP_ENCODED_OK_V1;
}
static int valid_view(const NgpMemoryMp3ViewV1*v,size_t bytes){
 const NgpMp3EnvelopeV1*e=&v->envelope;
 return v->pcm&&e->source_bytes==bytes&&e->channels>=1&&e->channels<=2&&e->rate>=8000&&e->rate<=192000&&e->pcm_frames>0&&e->encoded_frames<=NGP_MP3_MAX_FRAMES_V1&&
  ((e->mpeg_version==1&&e->rate==44100&&(uint64_t)e->encoded_frames*1152==e->pcm_frames)||(e->mpeg_version==2&&e->rate==22050&&e->channels==2&&(uint64_t)e->encoded_frames*576==e->pcm_frames))&&
  e->audio_offset<=e->source_bytes&&e->audio_bytes<=e->source_bytes-e->audio_offset&&e->encoded_frames>0&&
  (uint64_t)e->pcm_frames*e->channels*2==e->pcm_bytes&&e->pcm_bytes<=NGP_MP3_MAX_BYTES_V1&&span(v->pcm,e->pcm_bytes)&&
  v->source_end_reached==1&&v->bitstream_integrity_verified==0&&v->gapless_applied==0;
}
static int bind_mp3(NgpVoiceServiceV1*service,NgpVoiceHandleV1 token,const void*source,size_t bytes,NgpEncodedVoiceBindReportV1*r){
 NgpMemoryMp3V1*owned=NULL;int code=ngp_memory_mp3_v1_decode(source,bytes,&owned);r->decode_status=code;
 if(code){
  first_error(r,NGP_ENCODED_ORIGIN_DECODE_V1);
  if(owned){poison(r,owned,1);return NGP_ENCODED_POISONED_V1;} /* Contradictory result: no dereference or retry. */
  return(code==NGP_MP3_INVALID_V1||code==NGP_MP3_UNSUPPORTED_V1||code==NGP_MP3_LIMIT_V1)?NGP_ENCODED_FILE_REFUSED_V1:NGP_ENCODED_CODEC_FAILED_V1;
 }
 if(!owned){first_error(r,NGP_ENCODED_ORIGIN_DECODE_V1);poison(r,NULL,1);return NGP_ENCODED_POISONED_V1;}
 r->decoder_created=1;NgpMemoryMp3ViewV1 view={0};code=ngp_memory_mp3_v1_view(owned,&view);r->view_status=code;int result=NGP_ENCODED_CODEC_FAILED_V1;
 if(code||!valid_view(&view,bytes)){
  first_error(r,NGP_ENCODED_ORIGIN_VIEW_V1);poison(r,NULL,0); /* Authenticated producer invariant; safe unborrowed cleanup still follows. */
 }else{
  r->mp3=view.envelope;r->mp3_info_valid=1;r->source_end_reached=view.source_end_reached;r->bitstream_integrity_verified=view.bitstream_integrity_verified;r->gapless_applied=view.gapless_applied;
  NgpVoicePcmV1 pcm={1,sizeof(pcm),NGP_VOICE_PCM_S16_V1,view.envelope.channels,view.envelope.rate,0,view.pcm,view.envelope.pcm_bytes,view.envelope.pcm_frames};
  code=ngp_voice_service_v1_bind_pcm(service,token,&pcm,&r->voice_after);r->bind_status=code;r->copy_committed=code==0;
  result=code?NGP_ENCODED_VOICE_FAILED_V1:NGP_ENCODED_OK_V1;if(code)first_error(r,NGP_ENCODED_ORIGIN_BIND_V1);
 }
 /* Pinned bind NEVER borrows PCM: early refusal does not access/retain it;
  * accepted path memcpy's to pending own storage before any library init.
  * Hence this one release is safe after either bind result or unborrowed view
  * failure. No source/decoder pointer is stored by the service. */
 release_decoder(&owned,r);
 return guard.poisoned?NGP_ENCODED_POISONED_V1:result;
}
int ngp_encoded_voice_bind_v1(NgpVoiceServiceV1*service,NgpVoiceHandleV1 token,const void*source,size_t bytes,uint32_t kind,NgpEncodedVoiceBindReportV1*r){
 if(!r||r->version!=1||r->size!=sizeof(*r)||!span(r,sizeof(*r)))return NGP_ENCODED_ARGUMENT_V1;
 /* Reject overlap BEFORE report writes could mutate encoded input. Valid live
  * disjoint native objects are still caller preconditions, not proved here. */
 if(span(source,bytes)&&overlap(source,bytes,r,sizeof(*r)))return NGP_ENCODED_ARGUMENT_V1;
 initialize(r,bytes,kind);
 if(!service||!token||!span(source,bytes)||(kind!=NGP_ENCODED_WAVE_PCM_V1&&kind!=NGP_ENCODED_MP3_V1))return argument(r,NGP_ENCODED_ARGUMENT_V1);
 if(bytes>NGP_WAVE_MAX_BYTES_V1||bytes>NGP_MP3_MAX_BYTES_V1)return argument(r,NGP_ENCODED_LIMIT_V1);
 if(guard.poisoned){first_error(r,NGP_ENCODED_ORIGIN_GUARD_V1);return complete(r,NGP_ENCODED_POISONED_V1,0);}
 if(guard.busy){first_error(r,NGP_ENCODED_ORIGIN_GUARD_V1);return complete(r,NGP_ENCODED_BUSY_V1,0);}
 guard.busy=1;int code=ngp_voice_service_v1_snapshot(service,token,&r->voice_before);r->admission_status=code;
 if(code){first_error(r,NGP_ENCODED_ORIGIN_ADMISSION_V1);return complete(r,NGP_ENCODED_VOICE_FAILED_V1,1);}
 code=kind==NGP_ENCODED_WAVE_PCM_V1?bind_wave(service,token,source,bytes,r):bind_mp3(service,token,source,bytes,r);
 return complete(r,code,1);
}
