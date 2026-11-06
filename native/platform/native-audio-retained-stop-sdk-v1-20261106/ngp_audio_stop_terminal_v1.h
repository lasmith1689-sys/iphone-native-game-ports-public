/* Own INTERNAL retained capsule helper; never exposed as a guest/native handle. */
#ifndef NGP_AUDIO_STOP_TERMINAL_V1_H
#define NGP_AUDIO_STOP_TERMINAL_V1_H
#include <stdint.h>
#include <stdatomic.h>
typedef struct { _Atomic uint32_t outcome; } NgpAudioStopTerminalV1;
/* The sole broker construction initializes outcome to0 exactly once BEFORE
 * publication. No reset/init/destroy API exists. Valid retained native object. */
uint32_t ngp_audio_stop_terminal_v1_publish(NgpAudioStopTerminalV1*,uint32_t);
uint32_t ngp_audio_stop_terminal_v1_read(const NgpAudioStopTerminalV1*);
#endif
