/* OWN source selection: replace old mixer implementation; never co-link both.
 * Exact pinned miniaudio header/license supplied by future reviewed recipe. */
#define MINIAUDIO_IMPLEMENTATION
#include "ngp_audio_shared_profile_v1.h"
#define NGP_AUDIO_SHARED_IMPLEMENTATION_V1 1
#include "ngp_memory_mp3_v1.c"
