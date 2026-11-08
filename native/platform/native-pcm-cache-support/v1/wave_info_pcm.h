#ifndef COD_MILES_WAVE_INFO_PCM_V1_H
#define COD_MILES_WAVE_INFO_PCM_V1_H
#include <stdint.h>
#include <stddef.h>
typedef struct {uint32_t word[9];} CodMilesWaveInfoV1;
/* Owned immutable source and live disjoint output. Caller proves the mapping
 * between this native source and its guest address. This helper does not grant
 * guest memory access or a cache lease. Zero means success; nonzero refusal
 * preserves output. A future guest ABI adapter must return original EAX=1
 * only after successful commit to independently checked guest output memory. */
int cod_miles_wave_info_pcm_v1(const void *source,size_t bytes,
                              uint32_t guest_address,CodMilesWaveInfoV1 *out);
#endif
