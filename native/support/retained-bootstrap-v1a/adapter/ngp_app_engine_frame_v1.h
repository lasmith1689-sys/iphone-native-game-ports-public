/* PRIVATE fixed-target source candidate. No runtime/SDK/functional admission. */
#ifndef NGP_APP_ENGINE_FRAME_V1_H
#define NGP_APP_ENGINE_FRAME_V1_H
#include "ngp_app_engine_adapter_v1.h"
enum {NGP_APP_FRAME_REPORT_VERSION_V1=1,NGP_APP_FRAME_ADAPTER_ORIGIN_V1=17};
enum {NGP_APP_FRAME_INTEGRITY_V1=-10101,NGP_APP_FRAME_MEMORY_V1=-10102,
 NGP_APP_FRAME_EXHAUSTED_V1=-10103,NGP_APP_FRAME_REENTRANT_V1=-10104,
 NGP_APP_FRAME_RANGE_V1=-10105};
enum {NGP_APP_FRAME_STAGE_PRIOR_V1=1,NGP_APP_FRAME_STAGE_REENTRANT_V1=2,
 NGP_APP_FRAME_STAGE_PHASE_V1=3,NGP_APP_FRAME_STAGE_ARGUMENT_V1=4,
 NGP_APP_FRAME_STAGE_EXHAUSTED_V1=5,NGP_APP_FRAME_STAGE_MEMORY_V1=6,
 NGP_APP_FRAME_STAGE_COMMON_V1=7,NGP_APP_FRAME_STAGE_IDLE_V1=8,
 NGP_APP_FRAME_STAGE_POWER_V1=9,NGP_APP_FRAME_STAGE_ENTRY_V1=10,
 NGP_APP_FRAME_STAGE_ROOT_V1=11,NGP_APP_FRAME_STAGE_RETURN_V1=12};
enum {NGP_APP_FRAME_OBS_COMMON_V1=1,NGP_APP_FRAME_OBS_POWER_V1=2,
 NGP_APP_FRAME_OBS_ENTRY_V1=4,NGP_APP_FRAME_OBS_ROOT_V1=8,
 NGP_APP_FRAME_OBS_RETURN_V1=16,NGP_APP_FRAME_OBS_BUDGET_V1=32,
 NGP_APP_FRAME_OBS_TRAP_V1=64,NGP_APP_FRAME_COMMON_DISCARDED_V1=128};
typedef struct {
 uint32_t version,size,valid,stage;uint64_t attempt;
 int32_t result,current_status;uint32_t current_kind,first_failure_origin,first_failure_kind;
 int32_t first_failure_status,power_status,root_status;
 uint32_t common_esp,common_return_pc,before_esp,entry_esp,after_esp,return_pc,eax,trap_address;
 uint32_t saved_before[4],saved_after[4];int64_t requested_budget,remaining_budget;
} NgpAppFrameReportV1;
/* Same genuine worker and retained session/MEM/code only, under independently
 * admitted functional/native-guard/guest-memory/lifecycle contracts. No caller
 * readiness Boolean, CPU pointer, generic target, poll or presentation API.
 * Optional report is trusted writable retained native storage disjoint from
 * session/CPU/MEM and all live aliases; foreign/main refusal leaves it untouched.
 * Attempt counts are adapter calls, never guest/rendered frame counts. Current
 * root/partial-register observations are distinct from the retained first error.
 * Trap/budget getters can be stale if the root refused before host entry; their
 * observation bits do not assert a fresh trap or measured cost in that case.
 * Any refusal is terminal; no repair/retry/restart/G2 release. */
int32_t ngp_app_engine_frame_v1(NgpAppEngineSessionV1 *,NgpPowerWorkerV1 *,
 int64_t positive_budget,NgpAppFrameReportV1 *optional_report);
#endif
