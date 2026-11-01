#ifndef NGP_HEADLESS_MIXER_PROFILE_V1_H
#define NGP_HEADLESS_MIXER_PROFILE_V1_H
/* Pinned miniaudio0.11.23 only. No device, file decoder, resource worker or
 * dynamic library service. NO_THREADING does not promise a lock-free graph.
 * All graph/control/read operations here are serialized by the caller. */
#define MA_NO_DEVICE_IO
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_RUNTIME_LINKING
#define MA_NO_THREADING
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_FLAC
#define MA_NO_WAV
#define MA_NO_MP3
#include "miniaudio.h"
#endif
