#ifndef NGP_AUDIO_WORKER_RUNTIME_V1_H
#define NGP_AUDIO_WORKER_RUNTIME_V1_H
#include "ngp_device_mixer_v1.h"
#include "ngp_positional_provider_v1.h"
typedef struct NgpAudioWorkerRuntimeV1 NgpAudioWorkerRuntimeV1;
enum {NGP_AUDIO_WORKER_OK=0,NGP_AUDIO_WORKER_ARGUMENT=1,
 NGP_AUDIO_WORKER_OWNER=2,NGP_AUDIO_WORKER_PENDING=3,
 NGP_AUDIO_WORKER_STATE=4,NGP_AUDIO_WORKER_ALLOC=5,
 NGP_AUDIO_WORKER_BROKER=6,NGP_AUDIO_WORKER_BACKEND=7,
 NGP_AUDIO_WORKER_RETAINED=8,NGP_AUDIO_WORKER_WAIT=9};
enum {NGP_AUDIO_WORKER_STARTING=1,NGP_AUDIO_WORKER_READY=2,
 NGP_AUDIO_WORKER_FAILED_RETAINED=3,NGP_AUDIO_WORKER_STOPPING_RETAINED=4,
 NGP_AUDIO_WORKER_STOPPED_RETAINED=5};
typedef struct {
 uint32_t version,size,status,phase,worker_observation;
 uint32_t ordinary_reserved,positional_reserved,stream_reserved;
 uint32_t voice_prepared,gate_enabled,stop_request_returned;
 int32_t backend_status,stop_request_status;
 uint64_t intent_epoch;
} NgpAudioWorkerReportV1;
typedef struct {
 NgpVoiceServiceV1*service;NgpVoiceTokensV1*ordinary;
 NgpPositionalProviderV1*positional;NgpVoiceTokensV1*streams;
} NgpAudioWorkerContextsV1;
/* Valid disjoint caller objects, report.version1/size, output starts NULL/zero.
 * One attempt on the actual persistent nonmain backend worker. Broker is the
 * app's retained actual NGPDeviceMixerBrokerV1, passed as a native bridge value.
 * A published context persists on failure. Counts are acknowledged tracking,
 * never liveness/free/quiescence certificates. No destroy/retry API.
 */
int ngp_audio_worker_v1_open(void*broker,NgpAudioWorkerRuntimeV1**,NgpAudioWorkerReportV1*);
/* Returned contexts are synchronous worker borrows, not guest handles or a
 * guarantee that a later operation/device callback will succeed. */
int ngp_audio_worker_v1_borrow(NgpAudioWorkerRuntimeV1*,NgpAudioWorkerContextsV1*,NgpAudioWorkerReportV1*);
/* Separate stop report: the preexisting open/borrow report ABI is unchanged.
 * All mutable diagnostics are observed only by the persistent nonmain owner.
 * first_worker_status/backend_status never clear; close_ack is historical and
 * remains valid after intentional later broker invalidation. */
typedef struct {
 uint32_t version,size,status,phase,worker_observation;
 uint32_t stop_consumed,close_attempted,close_ack,request_returned;
 int32_t submission_status;
 uint32_t broker_outcome,first_worker_status;
 int32_t backend_status;
 uint64_t intent_epoch;
} NgpAudioWorkerStopReportV1;
/* Same valid/disjoint report and permanent worker/retained native object
 * preconditions as open/borrow. No synchronous wait, retry, free or G2. */
int ngp_audio_worker_v1_request_stop(NgpAudioWorkerRuntimeV1*,NgpAudioWorkerStopReportV1*);
int ngp_audio_worker_v1_poll_stop(NgpAudioWorkerRuntimeV1*,NgpAudioWorkerStopReportV1*);
/* Cooperative worker-only wait. Same64-byte stop report/retained ownership.
 * Actual monotonic5s deadline, <=2ms sleeps,4096claimed cycles; no hard wall-time
 * theorem. First wait fault quarantines permanently, including later old polls.
 * A same-epoch successful duplicate reobserves actual ACK; no close/requeue/free. */
int ngp_audio_worker_v1_wait_stop(NgpAudioWorkerRuntimeV1*,NgpAudioWorkerStopReportV1*);
int ngp_audio_worker_broker_wait_admit_v1(void*,NgpVoiceServiceV1*,uint64_t);
/* Production definitions are in the actual Objective-C bridge. Controlled
 * tests link explicitly synthetic definitions; no caller readiness callbacks. */
int ngp_audio_worker_broker_query_v1(void*,NgpDeviceMixerFactsV1*);
int ngp_audio_worker_broker_request_stop_v1(void*);
int ngp_audio_worker_broker_query_stop_v1(void*,NgpDeviceMixerStopObservationV1*);
#endif
