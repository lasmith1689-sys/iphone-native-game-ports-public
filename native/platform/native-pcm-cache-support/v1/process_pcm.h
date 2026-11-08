#ifndef COD_MILES_PROCESS_PCM_V1_H
#define COD_MILES_PROCESS_PCM_V1_H
#include <stddef.h>
#include <stdint.h>
typedef struct cod_miles_process_result_v1 {
    uint32_t written_bytes;
    uint32_t record[30];
} cod_miles_process_result_v1;
/* One fresh, immutable PCM source; default filtered conversion only.
 * record[1]==record[8] names the first source byte in the caller's guest map.
 * Native source/output/result lifetimes and guest mapping remain caller-owned.
 * capacity must equal the exact size helper result (including 256 padding).
 * Successful conversion preserves bytes the original mixer did not write.
 * No allocation, native audio, guest grant, SDK or dispatch is provided here.
 * Zero succeeds. Refusal preserves output and result completely. */
int cod_miles_process_pcm_v1(const unsigned char *source, size_t source_bytes,
    const uint32_t record[30], uint32_t target_rate, uint32_t target_format,
    unsigned char *output, size_t capacity, cod_miles_process_result_v1 *result);
#endif
