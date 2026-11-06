/* EXPLICIT TEST ONLY replacement OS provider. Never production-linked. */
#include "test_wait_provider.h"
#include <errno.h>
NgpWaitTestState ngp_wait_test;
int ngp_audio_stop_os_now_v1(uint64_t*out,int32_t*error){
 NgpWaitTestState*t=&ngp_wait_test;t->now_calls++;*error=0;
 if(t->mode==11||t->mode==27||t->fail_next_now||(t->mode==28&&t->now_calls==2)){*error=EIO;return NGP_STOP_OS_SYSCALL_V1;}
 if(t->mode==13)return NGP_STOP_OS_TIMESPEC_V1;
 if(t->mode==14){*out=UINT64_MAX-1;return 0;}
 *out=t->now;return 0;
}
int ngp_audio_stop_os_sleep_v1(uint64_t ns,int32_t*error){
 NgpWaitTestState*t=&ngp_wait_test;t->sleeps++;t->last_interval=ns;if(ns>t->maximum_interval)t->maximum_interval=ns;*error=0;
 if(!ns||ns>2000000)return NGP_STOP_OS_INTERVAL_V1;
 if(t->mode==6){*error=EINTR;return NGP_STOP_OS_INTERRUPTED_V1;}
 if(t->mode==9){*error=EIO;return NGP_STOP_OS_SYSCALL_V1;}
 if(t->mode==10)return 99;
 if(t->mode==7||t->mode==8)t->now=t->deadline;
 return 0;
}
