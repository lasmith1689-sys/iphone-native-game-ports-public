#ifndef NGP_TEST_WAIT_PROVIDER_H
#define NGP_TEST_WAIT_PROVIDER_H
#include "ngp_audio_stop_os_v1.h"
typedef struct {unsigned mode,now_calls,sleeps,queries,admissions,fail_next_now;uint64_t now,deadline,last_interval,maximum_interval;} NgpWaitTestState;
extern NgpWaitTestState ngp_wait_test;
#endif
