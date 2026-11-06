#ifndef NGP_TEST_WAIT_API_H
#define NGP_TEST_WAIT_API_H
#include "ngp_audio_worker_runtime_v1.h"
/* Explicit test-only API baseline. Never a production definition. */
int ngp_audio_worker_v1_wait_stop(NgpAudioWorkerRuntimeV1*,NgpAudioWorkerStopReportV1*);
int ngp_test_wait_now_v1(uint64_t*);
#endif
