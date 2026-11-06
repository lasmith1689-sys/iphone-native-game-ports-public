/* OWN TEST ONLY typed interception. Never in production selection/image. */
#ifndef NGP_ENCODED_TEST_FAULT_H
#define NGP_ENCODED_TEST_FAULT_H
#include "ngp_encoded_voice_bind_v1.h"
extern unsigned test_mode,test_decodes,test_views,test_releases,test_nested,test_sound_inits;
extern NgpVoiceServiceV1*test_service;
extern NgpVoiceHandleV1 test_handle;
extern const void*test_original;
extern size_t test_original_bytes;
int actual_memory_decode(const void*,size_t,NgpMemoryMp3V1**);
int actual_memory_view(const NgpMemoryMp3V1*,NgpMemoryMp3ViewV1*);
int actual_memory_release(NgpMemoryMp3V1**);
int actual_prefix_decode(const void*,size_t,NgpMemoryMp3V1**,NgpMp3PaddingV1*);
#endif
