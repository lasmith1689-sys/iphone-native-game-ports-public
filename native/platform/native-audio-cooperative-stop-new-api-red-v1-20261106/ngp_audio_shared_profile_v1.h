#ifndef NGP_AUDIO_SHARED_PROFILE_V1_H
#define NGP_AUDIO_SHARED_PROFILE_V1_H
/* ONE future implementation for serialized headless graph and worker codec.
 * MP3 worker calls only embedded frame API. Generic file APIs are not claimed
 * absent. No device/resource worker/runtime loading; graph stays caller-serial. */
#define MA_NO_DEVICE_IO
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_RUNTIME_LINKING
#define MA_NO_THREADING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_FLAC
#define MA_NO_WAV
#define MA_NO_VORBIS
#include "miniaudio.h"
#endif
