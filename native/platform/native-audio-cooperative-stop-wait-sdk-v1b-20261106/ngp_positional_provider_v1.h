#ifndef NGP_POSITIONAL_PROVIDER_V1_H
#define NGP_POSITIONAL_PROVIDER_V1_H
#include "ngp_voice_tokens_v1.h"
typedef struct NgpPositionalProviderV1 NgpPositionalProviderV1;
enum {NGP_POSITIONAL_OK=0,NGP_POSITIONAL_ARGUMENT=1,NGP_POSITIONAL_OWNER=2,
 NGP_POSITIONAL_STATE=3,NGP_POSITIONAL_TOKEN=4,NGP_POSITIONAL_LIMIT=5,
 NGP_POSITIONAL_BACKEND=6,NGP_POSITIONAL_ALLOC=7};
enum {NGP_POSITIONAL_OPENING=1,NGP_POSITIONAL_LIVE=2,
 NGP_POSITIONAL_FAILED=3,NGP_POSITIONAL_CLOSED=4};
/* active is last provider-acknowledged token tracking. In LIVE it counts owned
 * loans; FAILED may lag token-pool retirements after partial release/close.
 * It is never a backend-live, quiescence, device or free/retry certificate.
 * FAILED rejects all further mutation; retained tracking is diagnostic only. */
typedef struct {
 uint32_t version,size,status,phase,capacity,active,worker_observation;
 int32_t backend_status;float metres_per_unit;
} NgpPositionalReportV1;
/* Live/disjoint caller objects; report starts version1/sizeof(report). Open's
 * output starts NULL; any published context persists even on partial failure.
 * One process attempt on the actual persistent backend worker. All later calls
 * use that same live pthread. Foreign reports do not read mutable loan state.
 * Resolve output starts0: borrowed native VALUE for synchronous bind/play/seek/
 * gain use only. Do not externally release it or change its spatial position;
 * all positions/listener/unit factor go through this provider.
 * This is no device/Miles API, arbitrary-pointer check or reset/destroy API. */
int ngp_positional_v1_open(NgpVoiceServiceV1*,NgpPositionalProviderV1**,NgpPositionalReportV1*);
int ngp_positional_v1_acquire(NgpPositionalProviderV1*,uint32_t*,NgpPositionalReportV1*);
int ngp_positional_v1_resolve(NgpPositionalProviderV1*,uint32_t,uint64_t*,NgpPositionalReportV1*);
int ngp_positional_v1_release(NgpPositionalProviderV1*,uint32_t,NgpPositionalReportV1*);
int ngp_positional_v1_set_position(NgpPositionalProviderV1*,uint32_t,float,float,float,NgpPositionalReportV1*);
int ngp_positional_v1_set_listener(NgpPositionalProviderV1*,float,float,float,NgpPositionalReportV1*);
int ngp_positional_v1_set_factor(NgpPositionalProviderV1*,float,NgpPositionalReportV1*);
int ngp_positional_v1_close(NgpPositionalProviderV1*,NgpPositionalReportV1*);
#endif
