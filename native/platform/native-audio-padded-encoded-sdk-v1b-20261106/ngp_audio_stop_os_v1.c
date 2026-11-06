/* Genuine Apple monotonic clock/sleep provider. No retry or hidden wait loop. */
#include "ngp_audio_stop_os_v1.h"
#include <time.h>
#include <errno.h>
int ngp_audio_stop_os_now_v1(uint64_t *ns,int32_t *native_error){
 if(!ns||!native_error)return NGP_STOP_OS_ARGUMENT_V1;
 struct timespec t={0};int rc=clock_gettime(CLOCK_MONOTONIC,&t);int error=errno;
 *native_error=rc?(int32_t)error:0;
 if(rc)return NGP_STOP_OS_SYSCALL_V1;
 if(t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return NGP_STOP_OS_TIMESPEC_V1;
 uint64_t seconds=(uint64_t)t.tv_sec,nanos=(uint64_t)t.tv_nsec;
 if(seconds>(UINT64_MAX-nanos)/UINT64_C(1000000000))return NGP_STOP_OS_OVERFLOW_V1;
 *ns=seconds*UINT64_C(1000000000)+nanos;return NGP_STOP_OS_OK_V1;
}
int ngp_audio_stop_os_sleep_v1(uint64_t ns,int32_t *native_error){
 if(!native_error)return NGP_STOP_OS_ARGUMENT_V1;
 *native_error=0;if(!ns||ns>UINT64_C(2000000))return NGP_STOP_OS_INTERVAL_V1;
 struct timespec t={0,(long)ns};int rc=nanosleep(&t,NULL);int error=errno;
 if(!rc)return NGP_STOP_OS_OK_V1;
 *native_error=(int32_t)error;
 if(rc==-1&&error==EINTR)return NGP_STOP_OS_INTERRUPTED_V1;
 return NGP_STOP_OS_SYSCALL_V1;
}
