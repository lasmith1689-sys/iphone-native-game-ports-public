/* TEST ONLY library-error seam. All prepare/ownership/copy paths are actual C. */
#include "ngp_audio_shared_profile_v1.h"
ma_result test_sound_init(ma_engine*,ma_data_source*,ma_uint32,ma_sound_group*,ma_sound*);
#define ma_sound_init_from_data_source test_sound_init
#include "ngp_voice_service_v1.c"
