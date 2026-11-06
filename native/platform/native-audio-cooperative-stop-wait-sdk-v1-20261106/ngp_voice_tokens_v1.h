#ifndef NGP_VOICE_TOKENS_V1_H
#define NGP_VOICE_TOKENS_V1_H
#include "ngp_voice_service_v1.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NgpVoiceTokensV1 NgpVoiceTokensV1;
enum {NGP_TOKENS_OK=0,NGP_TOKENS_ARGUMENT=1,NGP_TOKENS_OWNER=2,
 NGP_TOKENS_STATE=3,NGP_TOKENS_TOKEN=4,NGP_TOKENS_LIMIT=5,
 NGP_TOKENS_BACKEND=6,NGP_TOKENS_RETAINED=7,NGP_TOKENS_ALLOC=8};
enum {NGP_TOKENS_RESERVED=1,NGP_TOKENS_STARTING=2,NGP_TOKENS_PREPARING=3,
 NGP_TOKENS_READY=4,NGP_TOKENS_FAILED=5,NGP_TOKENS_CLOSED=6};
typedef struct {
 uint32_t version,size,status,phase,kind,capacity,reserved_tracked,loans_tracked;
 int32_t backend_status;uint32_t worker_observation;
 NgpVoiceReportV1 voice;
} NgpVoiceTokensReportV1;
/* All output/report objects are live and disjoint native caller storage.
 * Initialize version1/sizeof(report); pool/native output starts NULL/zero.
 * Reserve is process-lived and once per kind; no reset/destroy/restart API.
 * Prepare/assign/resolve/release/close use the genuine backend's serial worker.
 * Resolve borrows a VALUE for synchronous native use only, never a pointer or
 * a game32 value. Finish that use before release/close. Foreign-thread reports
 * have worker_observation0 and do not read mutable entry/loan state.
 * Context/backend and creating pthread remain live; no pthread-ID reuse claim.
 * Failure after a backend effect is retained. Counts are acknowledged tracking,
 * not proof an uncertain backend resource survived. This is not a device gate,
 * a Miles implementation or a general arbitrary-pointer/alias sandbox.
 */
int ngp_voice_tokens_v1_reserve(uint32_t kind,NgpVoiceTokensV1 **,NgpVoiceTokensReportV1*);
int ngp_voice_tokens_v1_prepare(NgpVoiceTokensV1*,NgpVoiceServiceV1*,NgpVoiceTokensReportV1*);
int ngp_voice_tokens_v1_assign(NgpVoiceTokensV1*,uint32_t*,NgpVoiceTokensReportV1*);
int ngp_voice_tokens_v1_resolve(NgpVoiceTokensV1*,uint32_t,uint64_t*,NgpVoiceTokensReportV1*);
int ngp_voice_tokens_v1_release(NgpVoiceTokensV1*,uint32_t,NgpVoiceTokensReportV1*);
int ngp_voice_tokens_v1_close(NgpVoiceTokensV1*,NgpVoiceTokensReportV1*);
#ifdef __cplusplus
}
#endif
#endif
