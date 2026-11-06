#ifndef NGP_AUDIO_WORKER_RUNTIME_V1_H
#define NGP_AUDIO_WORKER_RUNTIME_V1_H
#include "ngp_device_mixer_v1.h"
#include "ngp_positional_provider_v1.h"
typedef struct NgpAudioWorkerRuntimeV1 NgpAudioWorkerRuntimeV1;
enum {NGP_AUDIO_WORKER_OK=0,NGP_AUDIO_WORKER_ARGUMENT=1,
 NGP_AUDIO_WORKER_OWNER=2,NGP_AUDIO_WORKER_PENDING=3,
 NGP_AUDIO_WORKER_STATE=4,NGP_AUDIO_WORKER_ALLOC=5,
 NGP_AUDIO_WORKER_BROKER=6,NGP_AUDIO_WORKER_BACKEND=7,
 NGP_AUDIO_WORKER_RETAINED=8};
enum {NGP_AUDIO_WORKER_STARTING=1,NGP_AUDIO_WORKER_READY=2,
 NGP_AUDIO_WORKER_FAILED_RETAINED=3};
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
/* Production definitions are in the actual Objective-C bridge. Controlled
 * tests link explicitly synthetic definitions; no caller readiness callbacks. */
int ngp_audio_worker_broker_query_v1(void*,NgpDeviceMixerFactsV1*);
int ngp_audio_worker_broker_request_stop_v1(void*);
#endif
