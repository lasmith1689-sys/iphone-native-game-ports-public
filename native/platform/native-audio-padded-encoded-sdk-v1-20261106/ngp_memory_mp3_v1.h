#ifndef NGP_MEMORY_MP3_V1_H
#define NGP_MEMORY_MP3_V1_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {
 NGP_MP3_OK_V1=0, NGP_MP3_INVALID_V1=1, NGP_MP3_UNSUPPORTED_V1=2,
 NGP_MP3_LIMIT_V1=3, NGP_MP3_ALLOCATION_V1=4, NGP_MP3_CODEC_V1=5,
 NGP_MP3_WORKER_V1=6, NGP_MP3_BUSY_V1=7,
 NGP_MP3_MAX_BYTES_V1=64*1024*1024, NGP_MP3_MAX_FRAMES_V1=65536,
 NGP_MP3_MAX_LIVE_V1 = 64
};
typedef struct {
 uint32_t source_bytes,audio_offset,audio_bytes,encoded_frames,pcm_frames,pcm_bytes;
 uint32_t rate,channels,mpeg_version,tag_prefix_bytes,tag_suffix_bytes;
} NgpMp3EnvelopeV1;
typedef struct NgpMemoryMp3V1 NgpMemoryMp3V1;
typedef struct {
 const int16_t *pcm;
 NgpMp3EnvelopeV1 envelope;
 uint32_t source_end_reached,bitstream_integrity_verified,gapless_applied;
} NgpMemoryMp3ViewV1;
/* Pure structural scan, not MP3 payload decoding/integrity verification.
 * Valid complete immutable source extent and valid disjoint result required.
 * All failures leave result unchanged. No retained compressed pointer. */
int ngp_mp3_envelope_v1_scan(const void*,size_t,NgpMp3EnvelopeV1*);
/* Apple non-main worker only. Caller owns immutable compressed extent until
 * return. Output slot must be valid/disjoint/NULL and is unchanged on failure.
 * Raw encoded-frame S16 PCM only: no gapless/delay/padding or CRC guarantee.
 * No source/guest/filesystem callbacks. No decoding on a render callback. */
int ngp_memory_mp3_v1_decode(const void*,size_t,NgpMemoryMp3V1**);
/* Creating worker only; borrowed const PCM remains valid until owner releases.
 * Owner must outlive all handles; stop/retire every borrower before release.
 * Caller keeps pointers/slots valid and serialized, with no stale-handle reuse. */
int ngp_memory_mp3_v1_view(const NgpMemoryMp3V1*,NgpMemoryMp3ViewV1*);
int ngp_memory_mp3_v1_release(NgpMemoryMp3V1**);
#ifdef __cplusplus
}
#endif
#endif
