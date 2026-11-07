/* PRIVATE own same-image source candidate. No build/runtime admission. */
#ifndef NGP_APP_ENGINE_ADAPTER_V1_H
#define NGP_APP_ENGINE_ADAPTER_V1_H
#include <stdint.h>
#include "cod_native_host_v4.h"
typedef struct NgpPowerWorkerV1 NgpPowerWorkerV1;
typedef struct NgpAppEngineSessionV1 NgpAppEngineSessionV1;
typedef struct {const uint8_t *bytes;uint32_t count;} NgpAppImageV1;
enum {NGP_APP_CREATED=1,NGP_APP_HOST_ATTEMPTED=2,NGP_APP_PREPARED=3,
 NGP_APP_BINDING=4,NGP_APP_ATTACHED=5,NGP_APP_COMMON_ENTERED=6,
 NGP_APP_COMMON_RETURNED=7};
#define NGP_APP_RETAINED_FAILURE UINT32_MAX
enum {NGP_APP_MAIN=-10001,NGP_APP_FOREIGN=-10002,NGP_APP_BAD_PHASE=-10003,
 NGP_APP_ARGUMENT=-10004,NGP_APP_ALLOCATION=-10005,NGP_APP_POINTER_RESULT=-10006,
 NGP_APP_BRIDGE_OBJECT=-10007,NGP_APP_BRIDGE_EXCEPTION=-10008};
enum {NGP_APP_STATUS_ADAPTER=1,NGP_APP_STATUS_HOST=2,
 NGP_APP_STATUS_SOURCE=3,NGP_APP_STATUS_POWER=4};
enum {NGP_APP_CPU=0,NGP_APP_MEM=1,NGP_APP_LOAD1=2,NGP_APP_LOAD2=3,
 NGP_APP_LOAD3=4,NGP_APP_LOAD4=5,NGP_APP_CONFIGURE=6,NGP_APP_SEAL=7,
 NGP_APP_PREPARE=8,NGP_APP_CONSUMER=9,NGP_APP_BRIDGE_BIND=10,
 NGP_APP_FACTORIES=11,NGP_APP_INPUT_BIND=12,NGP_APP_LOADING_BIND=13,
 NGP_APP_POWER=14,NGP_APP_COMMON=15,NGP_APP_STEPS=16,
 NGP_APP_ARGUMENT_OR_PHASE=16};
#define NGP_APP_NO_ORIGIN UINT32_MAX
typedef struct {
 uint32_t version,size,phase,host_attempted,attempted_mask;
 uint32_t last_origin,last_kind,failure_origin,failure_kind;
 int32_t last_status,failure_status;
 int32_t step_status[NGP_APP_STEPS];uint32_t step_kind[NGP_APP_STEPS];
} NgpAppEngineSnapshotV1;
/* One process bootstrap, one actual off-main worker. No destroy/retry/restart.
 * Valid readable native pointers are trusted; no arbitrary-pointer theorem.
 * A nonnull returned session remains resident even if CPU allocation failed. */
int32_t ngp_app_engine_session_create_v1(NgpAppEngineSessionV1 **out);
int32_t ngp_app_engine_prepare_v1(NgpAppEngineSessionV1 *,const char *profile,
 const NgpAppImageV1 images[4]);
const CodInputEngineV1 *ngp_app_engine_consumer_v1(NgpAppEngineSessionV1 *);
/* retained_binding is the actual final NGPAppSourceBindingV1 object, retained
 * by the future main controller. No arbitrary service table/readiness input.
 * Source/host/code/G2 owners remain retained after ANY attempted attachment. */
int32_t ngp_app_engine_attach_v1(NgpAppEngineSessionV1 *,void *retained_binding);
/* Genuine same-worker power context only; actual C power function is called.
 * All metadata/worker-prefix/frame admission remains an independent gate.
 * Return0 means only that this actual common-root call returned0. */
int32_t ngp_app_engine_common_v1(NgpAppEngineSessionV1 *,NgpPowerWorkerV1 *,
 const uint8_t *command,uint32_t count,int64_t budget);
/* Worker-only POD facts, no pointer addresses and no readiness claim. */
int32_t ngp_app_engine_snapshot_v1(NgpAppEngineSessionV1 *,NgpAppEngineSnapshotV1 *);
#endif
