#include "size_process_pcm.h"
#define LIMIT_BYTES UINT32_C(67108864)
_Static_assert(sizeof(CodMilesProcessRecordV1)==120, "record stride");
int cod_miles_pcm_process_size_v1(uint32_t rate, uint32_t format,
    uint32_t count, const CodMilesProcessRecordV1 *records, size_t bytes,
    uint32_t *out) {
    uintptr_t r=(uintptr_t)records, o=(uintptr_t)out;
    uint64_t maximum=0;
    if (!records || !out || count<1 || count>8 || format>3 ||
        rate<8000 || rate>192000 || bytes!=(size_t)count*120 ||
        r%_Alignof(CodMilesProcessRecordV1) || o%_Alignof(uint32_t) ||
        r>UINTPTR_MAX-(bytes-1) || o>UINTPTR_MAX-3 ||
        (r<=o+3 && o<=r+bytes-1)) return 1;
    for (uint32_t i=0;i<count;i++) {
        const uint32_t *w=records[i].word;
        uint32_t bits=w[4], channels=w[5], source_rate=w[3], n=w[2];
        if (w[0]!=1 || (bits!=8 && bits!=16) ||
            (channels!=1 && channels!=2) || source_rate<8000 ||
            source_rate>192000 || n>LIMIT_BYTES) return 1;
        uint32_t frame_bytes=(bits/8)*channels;
        if (n%frame_bytes) return 1;
        uint64_t numerator=(uint64_t)(n/frame_bytes)*rate;
        uint64_t frames=numerator/source_rate, remainder=numerator%source_rate;
        if (2*remainder>source_rate ||
            (2*remainder==source_rate && (frames&1))) frames++;
        if (frames>maximum) maximum=frames;
    }
    uint64_t total=maximum*(1+(format&1))*(1+((format>>1)&1))+256;
    if (total>LIMIT_BYTES) return 1;
    *out=(uint32_t)total;
    return 0;
}
int cod_miles_pcm_block_size_v1(uint32_t format, uint32_t value, uint32_t *word) {
    if (format>3 || !word || (uintptr_t)word%_Alignof(uint32_t)) return 1;
    *word=value;
    return 0;
}
