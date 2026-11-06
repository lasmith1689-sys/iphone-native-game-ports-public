/* Exactly one REAL implementation/TLS. Only three public names intercepted. */
#define ngp_memory_mp3_v1_decode actual_memory_decode
#define ngp_memory_mp3_v1_view actual_memory_view
#define ngp_memory_mp3_v1_release actual_memory_release
#include "ngp_audio_shared_implementation_v1.c"
