#ifndef NGP_FLS_PROTOCOL_V1_H
#define NGP_FLS_PROTOCOL_V1_H
#include <stdint.h>
#include "ngp_fls_environment_v1.h"
enum {NGP_FLS_OK=0,NGP_FLS_ARGUMENT=-1,NGP_FLS_OWNER=-2,NGP_FLS_STATE=-3,
 NGP_FLS_LIMIT=-4,NGP_FLS_PLATFORM=-5,NGP_FLS_RETAINED=-6,NGP_FLS_CALLBACK=-7,
 NGP_FLS_HOST=-8,NGP_FLS_UNSUPPORTED=-9};
enum {NGP_FLS_INIT=1,NGP_FLS_LIVE=2,NGP_FLS_RETIRING=3,NGP_FLS_FAILED=4,
 NGP_FLS_CLOSED=5,NGP_FLS_OWNER_EXITED=6};
enum {NGP_FLS_ALLOCATED=1,NGP_FLS_BEGIN_RETIRE=2,NGP_FLS_CLEAR_ACK=3,
 NGP_FLS_CALLBACK_ACK=4,NGP_FLS_DELETE_ACK=5,NGP_FLS_FAIL=6};
enum {NGP_FLS_BOX_CAPACITY=64};
typedef struct {uint32_t phase,acknowledgements;} NgpFlsPolicyV1;
/* Pure finite rules shared by production. Valid disjoint native storage.
 * A refusal preserves both output/state. FFFFFFFF is never an issued key. */
NGP_FLS_API int32_t ngp_fls_key_next_v1(uint32_t previous,uint32_t *output);
NGP_FLS_API int32_t ngp_fls_step_v1(NgpFlsPolicyV1 *state,uint32_t event);
#endif
