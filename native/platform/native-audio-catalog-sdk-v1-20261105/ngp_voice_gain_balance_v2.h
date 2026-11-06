/* OWN SOURCE CANDIDATE; native gain/balance only, no Miles/guest mapping. */
#ifndef NGP_VOICE_GAIN_BALANCE_V2_H
#define NGP_VOICE_GAIN_BALANCE_V2_H
#include "ngp_voice_service_v1.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {NGP_VOICE_UNSUPPORTED_V2=18};
typedef struct {
 uint32_t version,size;
 NgpVoiceReportV1 voice;
 float requested_linear_gain,requested_native_balance;
 uint32_t balance_mode,output_channels,gain_observation_valid;
} NgpVoiceGainBalanceReportV2;
/* Caller initializes ONLY outer version2 and size; embedded V1 is initialized
 * internally. All objects/spans valid/live/disjoint. Persistent engine worker
 * only. Stereo output and ordinary/stream-class only. Finite gain[0,16] and
 * balance[-1,1]. One close/drain/set-mode/set-volume/set-pan/finish transaction.
 * V1 set_volume still validates0..1; V1 requested_volume can reflect a prior
 * V2 gain>1. Its layout/signatures are unchanged; no clamping occurs.
 * Nonterminal pre-effect mono UNSUPPORTED18 and positional KIND5.
 * Report is cached POD; gain_observation_valid says the embedded handle/slot
 * has a matched cached pair, not readiness/presentation or failure recovery.
 * On foreign owner/bad token/context-only failure this flag is0. No library
 * query after failure, destroy, restart, pow, guest or callback control work. */
int ngp_voice_service_v2_set_gain_balance(NgpVoiceServiceV1*,NgpVoiceHandleV1,
 float linear_gain,float native_balance,NgpVoiceGainBalanceReportV2*);
#ifdef __cplusplus
}
#endif
#endif
