#ifndef NGP_MEMORY_MP3_INTERNAL_V1_H
#define NGP_MEMORY_MP3_INTERNAL_V1_H
#include "ngp_memory_mp3_v1.h"
typedef struct {
 uint32_t frame_bytes,samples,rate,channels,version,bitrate;
} NgpMp3FrameV1;
/* Internal fixed-header arithmetic. No allocation and unchanged result on fail. */
__attribute__((visibility("hidden")))
int ngp_mp3_frame_v1_header(const uint8_t*,size_t,NgpMp3FrameV1*);
#endif
