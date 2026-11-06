#ifndef NGP_MP3_ZERO_PREFIX_V1_H
#define NGP_MP3_ZERO_PREFIX_V1_H
#include "ngp_memory_mp3_v1.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {NGP_MP3_PADDING_NONE_V1=0,NGP_MP3_PADDING_ZERO_V1=1,
 NGP_MP3_PADDING_FF_ZERO_V1=2,NGP_MP3_MAX_PADDING_V1=4093};
typedef struct {
 uint32_t source_bytes,padding_bytes,suffix_bytes,kind;
} NgpMp3PaddingV1;
/* Explicit finite compatibility policy, not generic sync search. Entire source
 * is valid immutable borrowed storage through return (including all padding).
 * All output objects/slots are valid and disjoint from each other and the WHOLE
 * source. Failures preserve every output. Metadata records padding separately;
 * strict envelope/tag offsets and decoded view stay relative to the suffix.
 * Original64MiB source cap, unchanged strict profile/PCM/frame caps apply.
 * No source filesystem, asset mutation, allocation, retained compressed pointer
 * or later-candidate retry. Zero offset preserves supported strict ID3 policy. */
int ngp_mp3_zero_prefix_v1_scan(const void*,size_t,NgpMp3EnvelopeV1*,NgpMp3PaddingV1*);
/* Actual creating nonmain worker only. Existing TLS handle/view/release/quota
 * rules apply. *result initially NULL; on success unchanged v1 owns PCM and its
 * normal release API is required after borrowers retire. Decoder called ONCE.
 * A frame-valid suffix may still return CODEC; no fabricated PCM or resync. */
int ngp_mp3_zero_prefix_v1_decode(const void*,size_t,NgpMemoryMp3V1**,NgpMp3PaddingV1*);
#ifdef __cplusplus
}
#endif
#endif
