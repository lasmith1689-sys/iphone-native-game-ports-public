#ifndef COD_SIZE_PROCESS_PCM_V1_H
#define COD_SIZE_PROCESS_PCM_V1_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t word[30]; } CodMilesProcessRecordV1;
/* Pure bounded calculator, zero success. Caller owns live, immutable, aligned
 * records and a live aligned output word for this entire call. No PCM bytes
 * are dereferenced; this is not proof of a PCM source lease or processing.
 * Refusal leaves output unchanged. Output must not overlap any record. */
int cod_miles_pcm_process_size_v1(uint32_t target_rate, uint32_t output_format,
    uint32_t count, const CodMilesProcessRecordV1 *records, size_t records_bytes,
    uint32_t *output_bytes);
/* Only ordinary PCM metadata. Caller must prove the sample's exclusive owner,
 * serial, active epoch and live block-size word. Original null guest handle is
 * a separate adapter no-op. This helper never changes a backend or playback.
 * No frozen playback struct is extended here; no guest token is enabled. */
int cod_miles_pcm_block_size_v1(uint32_t pcm_format, uint32_t block_size,
    uint32_t *owned_block_size_word);
#endif
