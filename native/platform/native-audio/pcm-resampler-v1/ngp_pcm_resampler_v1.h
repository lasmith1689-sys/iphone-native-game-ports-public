#ifndef NGP_PCM_RESAMPLER_V1_H
#define NGP_PCM_RESAMPLER_V1_H
#include <stdint.h>
#include <stddef.h>
#define NGP_PCM_MAX_BYTES_V1 UINT32_C(67108864)
enum { NGP_PCM_OK_V1=0, NGP_PCM_INVALID_V1=1 };
typedef struct {
 uint32_t version,input_frames,source_rate,destination_rate,input_channels,output_channels;
 const uint8_t *source; size_t source_bytes; uint64_t cursor,total;
} NgpPcmResamplerV1;
typedef struct { uint32_t status,produced; uint64_t remaining; uint32_t finished; } NgpPcmResultV1;
/* Caller owns valid readable/writable native spans, immutable full source and
 * serialized state lifetime. No partial-input streaming or playback claim. */
int ngp_pcm_resampler_v1_init(NgpPcmResamplerV1 *,const void *,size_t,
 uint32_t frames,uint32_t source_rate,uint32_t input_channels,
 uint32_t destination_rate,uint32_t output_channels);
NgpPcmResultV1 ngp_pcm_resampler_v1_render(NgpPcmResamplerV1 *,void *,size_t);
#endif
