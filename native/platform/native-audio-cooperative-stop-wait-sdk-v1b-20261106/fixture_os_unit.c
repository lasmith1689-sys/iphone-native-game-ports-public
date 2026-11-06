/* Actual production provider body, explicitly renamed TEST-ONLY syscalls.
 * Not a real clock/sleep proof and never linked into the production image. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
static unsigned checks,clock_calls,sleep_calls;
static struct timespec scripted;
static int scripted_return,scripted_error;
static void check(int v,int line){checks++;if(!v){fprintf(stderr,"{\"status\":\"os-unit-failed\",\"line\":%d}\n",line);exit(101);}}
#define CHECK(v)check(!!(v),__LINE__)
static int test_clock(clockid_t id,struct timespec*out){CHECK(id==CLOCK_MONOTONIC);clock_calls++;*out=scripted;errno=scripted_error;return scripted_return;}
static int test_sleep(const struct timespec*in,struct timespec*remaining){CHECK(in&&in->tv_sec==0&&in->tv_nsec>0&&in->tv_nsec<=2000000&&remaining==NULL);sleep_calls++;errno=scripted_error;return scripted_return;}
#define clock_gettime test_clock
#define nanosleep test_sleep
#include "ngp_audio_stop_os_v1.c"
#undef clock_gettime
#undef nanosleep
static void invalid_now(time_t sec,long nsec,int rc,int error,int expected){
 scripted=(struct timespec){sec,nsec};scripted_return=rc;scripted_error=error;uint64_t out=99;int32_t e=99;
 CHECK(ngp_audio_stop_os_now_v1(&out,&e)==expected&&out==99);CHECK(e==(rc?error:0));
}
int main(void){
 CHECK(sizeof(time_t)==8);scripted=(struct timespec){0,100};uint64_t n=99;int32_t e=99;
 CHECK(!ngp_audio_stop_os_now_v1(&n,&e)&&n==100&&e==0);
 scripted=(struct timespec){(time_t)(UINT64_MAX/UINT64_C(1000000000)),(long)(UINT64_MAX%UINT64_C(1000000000))};CHECK(!ngp_audio_stop_os_now_v1(&n,&e)&&n==UINT64_MAX);
 invalid_now(-1,0,0,0,NGP_STOP_OS_TIMESPEC_V1);invalid_now(0,-1,0,0,NGP_STOP_OS_TIMESPEC_V1);invalid_now(0,1000000000L,0,0,NGP_STOP_OS_TIMESPEC_V1);
 invalid_now((time_t)(UINT64_MAX/UINT64_C(1000000000)+1),0,0,0,NGP_STOP_OS_OVERFLOW_V1);
 invalid_now(0,0,-1,EIO,NGP_STOP_OS_SYSCALL_V1);invalid_now(0,0,1,EIO,NGP_STOP_OS_SYSCALL_V1);invalid_now(0,0,-1,EINTR,NGP_STOP_OS_SYSCALL_V1);
 unsigned before=sleep_calls;CHECK(ngp_audio_stop_os_sleep_v1(0,&e)==NGP_STOP_OS_INTERVAL_V1);CHECK(ngp_audio_stop_os_sleep_v1(2000001,&e)==NGP_STOP_OS_INTERVAL_V1&&sleep_calls==before);
 scripted_return=0;scripted_error=EIO;CHECK(!ngp_audio_stop_os_sleep_v1(1,&e)&&!e);
 scripted_return=-1;scripted_error=EINTR;CHECK(ngp_audio_stop_os_sleep_v1(2000000,&e)==NGP_STOP_OS_INTERRUPTED_V1&&e==EINTR);
 scripted_error=EIO;CHECK(ngp_audio_stop_os_sleep_v1(1,&e)==NGP_STOP_OS_SYSCALL_V1&&e==EIO);
 scripted_return=1;scripted_error=EINTR;CHECK(ngp_audio_stop_os_sleep_v1(1,&e)==NGP_STOP_OS_SYSCALL_V1&&e==EINTR);
 before=clock_calls;CHECK(ngp_audio_stop_os_now_v1(NULL,&e)==NGP_STOP_OS_ARGUMENT_V1&&clock_calls==before);CHECK(ngp_audio_stop_os_now_v1(&n,NULL)==NGP_STOP_OS_ARGUMENT_V1&&clock_calls==before);before=sleep_calls;CHECK(ngp_audio_stop_os_sleep_v1(1,NULL)==NGP_STOP_OS_ARGUMENT_V1&&sleep_calls==before);
 printf("{\"status\":\"actual-provider-body-unit-passed\",\"checks\":%u,\"clockCalls\":%u,\"sleepCalls\":%u,\"syscallsExplicitlySynthetic\":true,\"realClockClaim\":false,\"device\":false,\"game\":false}\n",checks,clock_calls,sleep_calls);return 0;
}
