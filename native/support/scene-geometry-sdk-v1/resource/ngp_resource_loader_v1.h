/* PRIVATE fixed-resource interface. No engine/CPU/GPU or arbitrary callbacks. */
#ifndef NGP_RESOURCE_LOADER_V1_H
#define NGP_RESOURCE_LOADER_V1_H
#include <stdint.h>
typedef struct NgpResourceSetV1 NgpResourceSetV1;
typedef struct {const uint8_t *bytes;uint32_t count;} NgpResourceImageViewV1;
typedef struct {uint32_t version,size;int image_root_fd,asset_root_fd;const char *profile_label;} NgpResourceRootsV1;
typedef struct {
 uint32_t version,size,status,stage,file_index,files_hashed;
 int32_t native_error;uint64_t bytes_hashed;
 uint8_t expected_sha256[32],observed_sha256[32];
 uint32_t cwd_bytes;char canonical_cwd[255];
} NgpResourceReportV1;
enum {NGPR_OK=0,NGPR_ARGUMENT=1,NGPR_MAIN=2,NGPR_FOREIGN=3,NGPR_STATE=4,
 NGPR_ALLOC=5,NGPR_IO=6,NGPR_FILE=7,NGPR_HASH=8,NGPR_CHANGED=9,
 NGPR_NAMESPACE=10,NGPR_TIMEOUT=11,NGPR_CANCEL=12,NGPR_BUDGET=13,
 NGPR_CWD=14,NGPR_QUARANTINE=15};
enum {NGPR_STAGE_CREATE=1,NGPR_STAGE_NAMESPACE=2,NGPR_STAGE_FILE=3,
 NGPR_STAGE_CWD=4,NGPR_STAGE_BORROW=5,NGPR_STAGE_ABANDON=6};
/* Caller initializes every report.version=1,size=sizeof(report). Return0 means
 * that operation completed; errors are -NGPR_*. Report.status/native_error keep
 * the first context failure (including successful later abandon). Report from a
 * foreign/refused call is caller-owned only and does not change context state.
 * All pointers must be live correctly sized nonoverlapping native objects;
 * create's caller publishes only its returned process-owned context pointer.
 * This is not arbitrary-pointer validation. Opaque context never gets freed.
 */
int32_t ngp_resources_create_v1(const NgpResourceRootsV1*,NgpResourceSetV1**,NgpResourceReportV1*);
int32_t ngp_resources_validate_v1(NgpResourceSetV1*,NgpResourceReportV1*);
int32_t ngp_resources_enter_cwd_v1(NgpResourceSetV1*,NgpResourceReportV1*);
int32_t ngp_resources_borrow_for_prepare_v1(NgpResourceSetV1*,NgpResourceImageViewV1 out[4],NgpResourceReportV1*);
int32_t ngp_resources_cancel_v1(NgpResourceSetV1*);
int32_t ngp_resources_abandon_before_borrow_v1(NgpResourceSetV1*,NgpResourceReportV1*);
/* One create per creator thread; first nonmain validate claims one worker TLS
 * context permanently. All lifecycle methods then require that worker. Main
 * may atomically cancel a live published context, never read mutable fields,
 * wait, free or close its resources. Caller publishes/fences the returned
 * pointer before scheduling validate/cancel; no retained-pointer access after
 * process teardown. No pthread_t comparison or TLS-address identity is used.
 * BORROW is atomic-linearized against cancellation. After borrow resources/cwd
 * are process-retained; cancellation never releases them or admits engine use.
 * Pre-borrow abandon releases contents only after actual cwd restoration;
 * opaque cancel storage remains process-lived. A quarantined context remains.
 */
#endif
