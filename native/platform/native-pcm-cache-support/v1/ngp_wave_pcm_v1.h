#ifndef NGP_WAVE_PCM_V1_H
#define NGP_WAVE_PCM_V1_H
#include <stdint.h>
#include <stddef.h>
enum { NGP_WAVE_OK_V1=0,NGP_WAVE_INVALID_V1=1,NGP_WAVE_UNSUPPORTED_V1=2,NGP_WAVE_LIMIT_V1=3 };
enum { NGP_WAVE_MAX_BYTES_V1=64*1024*1024,NGP_WAVE_MAX_CHUNKS_V1=1024 };
typedef struct {
 uint32_t format_tag,data_offset,data_bytes,sample_rate,bits,channels;
 uint32_t sample_units,block_align,container_bytes,frames;
} NgpWavePcmV1;
/* Valid immutable owned source, disjoint result, serialized caller. No retained
 * pointer; source-relative offset must be translated through the owner ledger.
 * Invalid calls do not modify result. PCM8 is unsigned, PCM16 signed little-endian. */
int ngp_wave_pcm_v1_parse(const void*,size_t,NgpWavePcmV1*);
#endif
