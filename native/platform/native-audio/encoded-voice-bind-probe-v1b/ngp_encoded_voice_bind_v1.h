/* OWN SOURCE CANDIDATE. No native/SDK result or Miles stream API. */
#ifndef NGP_ENCODED_VOICE_BIND_V1_H
#define NGP_ENCODED_VOICE_BIND_V1_H
#include "ngp_voice_service_v1.h"
#include "ngp_wave_pcm_v1.h"
#include "ngp_memory_mp3_v1.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {NGP_ENCODED_WAVE_PCM_V1=1,NGP_ENCODED_MP3_V1=2};
enum {NGP_ENCODED_OK_V1=0,NGP_ENCODED_ARGUMENT_V1=1,
 NGP_ENCODED_FILE_REFUSED_V1=2,NGP_ENCODED_LIMIT_V1=3,
 NGP_ENCODED_VOICE_FAILED_V1=4,NGP_ENCODED_CODEC_FAILED_V1=5,
 NGP_ENCODED_RELEASE_FAILED_V1=6,NGP_ENCODED_POISONED_V1=7,
 NGP_ENCODED_BUSY_V1=8,NGP_ENCODED_NOT_RUN_V1=-2147483647};
enum {NGP_ENCODED_ORIGIN_NONE_V1=0,NGP_ENCODED_ORIGIN_ARGUMENT_V1=1,
 NGP_ENCODED_ORIGIN_ADMISSION_V1=2,NGP_ENCODED_ORIGIN_WAVE_V1=3,
 NGP_ENCODED_ORIGIN_DECODE_V1=4,NGP_ENCODED_ORIGIN_VIEW_V1=5,
 NGP_ENCODED_ORIGIN_BIND_V1=6,NGP_ENCODED_ORIGIN_RELEASE_V1=7,NGP_ENCODED_ORIGIN_GUARD_V1=8};
typedef struct {
 uint32_t version,size,status,first_error_origin,format_kind;
 int32_t admission_status,wave_status,decode_status,view_status,bind_status,release_status;
 uint32_t wave_info_valid,mp3_info_valid,decoder_created,release_attempted;
 uint32_t source_end_reached,bitstream_integrity_verified,gapless_applied;
 uint32_t copy_committed,release_confirmed,guard_state_observed,helper_poisoned,decoder_quarantined_identity;
 uint32_t decoder_retention_uncertain;
 uint64_t encoded_bytes;
 NgpWavePcmV1 wave;NgpMp3EnvelopeV1 mp3;
 NgpVoiceReportV1 voice_before,voice_after;
} NgpEncodedVoiceBindReportV1;
/* One synchronous persistent-worker operation. Caller initializes
 * outer version1/size only. All source/report/native objects live/disjoint;
 * source immutable and owned for the entire call, complete extent<=64MiB.
 * Validates real active native handle through actual snapshot first. No guest
 * path, filesystem, injected callback, readiness flag, playback or registry API.
 * WAVE translates checked source-relative data; MP3 uses real decode/view,
 * actual voice-owned PCM copy, then one release. All uncalled stage statuses
 * NOT_RUN; report preserves first stage error plus raw release outcome.
 * Inner voice status UINT32_MAX means unobserved (raw stage NOT_RUN).
 * Guard flags are observations only when guard_state_observed is1; invalid
 * outer/input arguments do not query TLS merely to report helper state.
 * Busy/poison refusals never clear another live helper invocation. Quarantined
 * identity is never dereferenced or used again. Public result has no pointer.
 * Unexpected release retains/quarantines and permanently poisons this worker's
 * helper admission; no retry/reset/free claim or rollback of a committed copy.
 * This neither implements AIL_open_stream nor proves whole app audio readiness. */
int ngp_encoded_voice_bind_v1(NgpVoiceServiceV1*,NgpVoiceHandleV1,
 const void*owned_encoded,size_t encoded_bytes,uint32_t format_kind,
 NgpEncodedVoiceBindReportV1*);
#ifdef __cplusplus
}
#endif
#endif
