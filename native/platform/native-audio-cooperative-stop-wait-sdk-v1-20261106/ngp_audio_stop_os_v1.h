#ifndef NGP_AUDIO_STOP_OS_V1_H
#define NGP_AUDIO_STOP_OS_V1_H
#include <stdint.h>
enum {NGP_STOP_OS_OK_V1=0,NGP_STOP_OS_SYSCALL_V1=1,
 NGP_STOP_OS_TIMESPEC_V1=2,NGP_STOP_OS_OVERFLOW_V1=3,
 NGP_STOP_OS_INTERVAL_V1=4,NGP_STOP_OS_INTERRUPTED_V1=5,
 NGP_STOP_OS_ARGUMENT_V1=6};
/* Private native provider. Valid disjoint outputs; failed now leaves ns unchanged.
 * Production links the real OS TU, never test providers/runtime callbacks. */
int ngp_audio_stop_os_now_v1(uint64_t *ns,int32_t *native_error);
int ngp_audio_stop_os_sleep_v1(uint64_t ns,int32_t *native_error);
#endif
