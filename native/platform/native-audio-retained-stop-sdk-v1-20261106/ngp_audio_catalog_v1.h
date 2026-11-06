#ifndef NGP_AUDIO_CATALOG_V1_H
#define NGP_AUDIO_CATALOG_V1_H
#include "ngp_audio_worker_runtime_v1.h"
#include <stddef.h>
typedef struct NgpAudioCatalogV1 NgpAudioCatalogV1;
enum {NGP_CATALOG_OK=0,NGP_CATALOG_ARGUMENT=1,NGP_CATALOG_OWNER=2,
 NGP_CATALOG_STATE=3,NGP_CATALOG_TOKEN=4,NGP_CATALOG_LIMIT=5,
 NGP_CATALOG_RUNTIME=6,NGP_CATALOG_BACKEND=7,NGP_CATALOG_ALLOC=8};
enum {NGP_CATALOG_BOUND=1,NGP_CATALOG_OPEN=2,NGP_CATALOG_CLOSED=3,
 NGP_CATALOG_FAILED_RETAINED=4};
typedef struct {
 uint32_t version,size,status,phase,worker_observation;
 uint32_t ordinary_loans_tracked,positional_loans_tracked;
 int32_t backend_status;float metres_per_unit;
} NgpAudioCatalogReportV1;
/* Valid live disjoint native caller objects; version1/sizeof(report), bind
 * output NULL, voice/resolve outputs0. Persistent live worker/context lifetime.
 * This is native support, not a guest ABI or positive legacy startup gate. */
int ngp_audio_catalog_v1_bind(NgpAudioWorkerRuntimeV1*,NgpAudioCatalogV1**,NgpAudioCatalogReportV1*);
/* Iterator0/1 only; available writes own name including NUL, end leaves name.
 * Provider4 is kind0; not a native pointer, driver or voice token. */
int ngp_audio_catalog_v1_enumerate(NgpAudioCatalogV1*,uint32_t*,uint32_t*,char*,size_t,NgpAudioCatalogReportV1*);
/* legacy_result changes to0 only on host success; never infer its value from
 * the host return status. All native pointers/values stay out of guest memory. */
int ngp_audio_catalog_v1_open_provider(NgpAudioCatalogV1*,uint32_t,uint32_t*,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_attribute(NgpAudioCatalogV1*,uint32_t,const char*,size_t,uint32_t*,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_set_distance_factor(NgpAudioCatalogV1*,uint32_t,float,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_acquire_ordinary(NgpAudioCatalogV1*,uint32_t*,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_acquire_positional(NgpAudioCatalogV1*,uint32_t,uint32_t*,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_resolve_voice(NgpAudioCatalogV1*,uint32_t,uint64_t*,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_release_voice(NgpAudioCatalogV1*,uint32_t,NgpAudioCatalogReportV1*);
int ngp_audio_catalog_v1_close_provider(NgpAudioCatalogV1*,uint32_t,NgpAudioCatalogReportV1*);
#endif
