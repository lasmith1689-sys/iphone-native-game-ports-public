/* OWN SOURCE CANDIDATE: no compiled/SDK/runtime claim. */
#ifndef NGP_VOICE_SERVICE_V1_H
#define NGP_VOICE_SERVICE_V1_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NgpVoiceServiceV1 NgpVoiceServiceV1;
typedef uint64_t NgpVoiceHandleV1;
enum {NGP_VOICE_ORDINARY_V1=1,NGP_VOICE_POSITIONAL_V1=2,NGP_VOICE_STREAM_V1=3};
enum {NGP_VOICE_PCM_U8_V1=1,NGP_VOICE_PCM_S16_V1=2,NGP_VOICE_PCM_F32_V1=3};
enum {NGP_VOICE_OK_V1=0,NGP_VOICE_ARGUMENT_V1=1,NGP_VOICE_OWNER_V1=2,
 NGP_VOICE_STATE_V1=3,NGP_VOICE_HANDLE_V1=4,NGP_VOICE_KIND_V1=5,
 NGP_VOICE_UNBOUND_V1=6,NGP_VOICE_LIMIT_V1=7,NGP_VOICE_ALLOC_V1=8,
 NGP_VOICE_LIBRARY_V1=9,NGP_VOICE_QUIESCENCE_V1=10,
 NGP_VOICE_CONCURRENT_RENDER_V1=11,NGP_VOICE_RENDER_V1=12,
 NGP_VOICE_CANCELLED_V1=13,NGP_VOICE_ROUTE_V1=14,
 NGP_VOICE_LIFECYCLE_V1=15,NGP_VOICE_CLOSED_V1=16,NGP_VOICE_GATED_V1=17};
enum {NGP_VOICE_RESERVED_V1=1,NGP_VOICE_PREPARING_V1=2,
 NGP_VOICE_READY_GATED_V1=3,NGP_VOICE_READY_OPEN_V1=4,
 NGP_VOICE_FAILED_RETAINED_V1=5,NGP_VOICE_CLOSED_RETAINED_V1=6};
enum {NGP_VOICE_MAX_RENDER_FRAMES_V1=4096};
/* Immutable actual broker negotiation. Supported output44100or48000Hz,
 * channels1or2, native interleaved Float32 render output. */
typedef struct {uint32_t version,size,sample_rate,channels;} NgpVoiceOutputV1;
/* Complete immutable caller span for this synchronous worker call. S16/F32
 * use native Apple little-endian representation. Bind COPIES into owned PCM. */
typedef struct {
 uint32_t version,size,format,channels,sample_rate,reserved;
 const void *bytes;uint64_t byte_count,frame_count;
} NgpVoicePcmV1;
typedef struct {
 uint32_t version,size,status,phase;int32_t library_status;
 uint32_t slot_index,kind,bound,playing,looping;
 uint64_t handle,generation,source_frames,source_cursor;
 /* render_sequence identifies cached worker observation, not a device clock. */
 uint64_t owned_pcm_bytes,engine_heap_bytes,render_sequence;
 uint64_t render_allocator_violations,zero_only_callbacks;
 float requested_pitch,requested_volume,position[3];
 uint32_t cursor_is_source_domain,seek_may_be_pending;
} NgpVoiceReportV1;
/* Initialize report.version=1,size=sizeof(report). All pointers/spans must be
 * valid live disjoint native objects; this is not arbitrary-pointer checking.
 * Main-safe reserve only allocates retained context/atomics: NO ma graph calls.
 * One process-wide attempt. A nonnull failed context must also be retained. */
int ngp_voice_service_v1_reserve(const NgpVoiceOutputV1*,NgpVoiceServiceV1**,NgpVoiceReportV1*);
/* Exactly one persistent nonmain worker claims/warms its TLS identity here.
 * Builds engine+77 real stopped one-frame OWN-silence data sources/sounds.
 * Leaves render gate CLOSED. READY is not broker/device activation. */
int ngp_voice_service_v1_prepare(NgpVoiceServiceV1*,NgpVoiceReportV1*);
int ngp_voice_service_v1_enable(NgpVoiceServiceV1*,NgpVoiceReportV1*);
int ngp_voice_service_v1_acquire(NgpVoiceServiceV1*,uint32_t kind,NgpVoiceHandleV1*,NgpVoiceReportV1*);
int ngp_voice_service_v1_bind_pcm(NgpVoiceServiceV1*,NgpVoiceHandleV1,const NgpVoicePcmV1*,NgpVoiceReportV1*);
int ngp_voice_service_v1_play(NgpVoiceServiceV1*,NgpVoiceHandleV1,NgpVoiceReportV1*);
int ngp_voice_service_v1_stop(NgpVoiceServiceV1*,NgpVoiceHandleV1,NgpVoiceReportV1*);
int ngp_voice_service_v1_seek(NgpVoiceServiceV1*,NgpVoiceHandleV1,uint64_t source_frame,NgpVoiceReportV1*);
int ngp_voice_service_v1_set_pitch(NgpVoiceServiceV1*,NgpVoiceHandleV1,float,NgpVoiceReportV1*);
int ngp_voice_service_v1_set_volume(NgpVoiceServiceV1*,NgpVoiceHandleV1,float,NgpVoiceReportV1*);
int ngp_voice_service_v1_set_looping(NgpVoiceServiceV1*,NgpVoiceHandleV1,uint32_t,NgpVoiceReportV1*);
int ngp_voice_service_v1_set_position(NgpVoiceServiceV1*,NgpVoiceHandleV1,float,float,float,NgpVoiceReportV1*);
int ngp_voice_service_v1_set_listener_position(NgpVoiceServiceV1*,float,float,float,NgpVoiceReportV1*);
int ngp_voice_service_v1_snapshot(NgpVoiceServiceV1*,NgpVoiceHandleV1,NgpVoiceReportV1*);
int ngp_voice_service_v1_release(NgpVoiceServiceV1*,NgpVoiceHandleV1,NgpVoiceReportV1*);
int ngp_voice_service_v1_close(NgpVoiceServiceV1*,NgpVoiceReportV1*);
/* Any thread; atomic/nonblocking only. Reason must CANCELLED/ROUTE/LIFECYCLE.
 * Irreversible. Does not wait, stop an Apple device, free or call miniaudio. */
int ngp_voice_service_v1_invalidate(NgpVoiceServiceV1*,uint32_t reason);
/* Callback only: exclusive caller-owned preallocated interleaved Float32 output,
 * capacity>=frames,1..4096 frames. No allocation/FS/codec/guest/ObjC work.
 * GATED17 is expected zero-only silence, not ready or successful graph mixing.
 * Valid admitted output is zeroed on a gated/refused/failed mix. INVALID buffer
 * shape causes no access; broker must silence its own valid hardware buffers.
 * Service protects graph mix, not caller scratch after this function returns. */
int ngp_voice_service_v1_render(NgpVoiceServiceV1*,float*,uint32_t frames,uint32_t capacity_frames);
/* No engine/data-source exposure, custom callbacks, destroy, reset or restart. */
#ifdef __cplusplus
}
#endif
#endif
