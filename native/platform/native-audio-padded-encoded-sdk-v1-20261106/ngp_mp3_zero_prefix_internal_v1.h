#ifndef NGP_MP3_ZERO_PREFIX_INTERNAL_V1_H
#define NGP_MP3_ZERO_PREFIX_INTERNAL_V1_H
#include "ngp_mp3_zero_prefix_v1.h"
__attribute__((visibility("hidden")))
int ngp_mp3_zero_prefix_v1_bounds(const void*,size_t,const void*,size_t,const void*,size_t);
__attribute__((visibility("hidden")))
int ngp_mp3_zero_prefix_v1_choose(const void*,size_t,NgpMp3PaddingV1*);
#endif
