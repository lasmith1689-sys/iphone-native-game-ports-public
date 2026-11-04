/* OWN candidate. Source only; no app, SDK or runtime admission. */
#ifndef NGP_POWER_LEASE_V1_H
#define NGP_POWER_LEASE_V1_H
#include <stdint.h>
typedef struct NgpPowerLeaseV1 NgpPowerLeaseV1;
typedef struct NgpPowerWorkerV1 NgpPowerWorkerV1;
typedef struct NgpPowerJoinReceiptV1 NgpPowerJoinReceiptV1;
/* All pointers are valid retained native objects. No bad-pointer validation.
 * One serialized engine pthread owns every use of the existing global seam. */
int ngp_power_worker_prepare_v1(NgpPowerLeaseV1 *,uint64_t incarnation,
 uint64_t generation,NgpPowerWorkerV1 **out);
int ngp_power_worker_bind_checkpoint_v1(NgpPowerWorkerV1 *);
int ngp_power_checkpoint_v1(void *);
void ngp_power_request_cancel_v1(NgpPowerLeaseV1 *);
int ngp_power_worker_unbind_checkpoint_v1(NgpPowerWorkerV1 *);
/* Non-main/non-engine joiner. Exactly one attempt, including failure; no
 * saved pthread API is used by a second attempt or after completed join. */
NgpPowerJoinReceiptV1 *ngp_power_join_worker_v1(NgpPowerWorkerV1 *);
#ifdef __OBJC__
#import <UIKit/UIKit.h>
@class CodInputSurfaceV3;
NS_ASSUME_NONNULL_BEGIN
__attribute__((visibility("default")))
@interface NgpPowerLeaseOwnerV1 : NSObject
- (instancetype)init NS_UNAVAILABLE;
- (nullable instancetype)initWithSource:(CodInputSurfaceV3 *)source
 incarnation:(uint64_t)incarnation generation:(uint64_t)generation;
/* Borrowed stable until joined retirement. Retain owner independently. */
- (NgpPowerLeaseV1 * _Nullable)borrowedLease;
/* Same worker only. Return0 means retained activation work was queued, never
 * that the lease is LIVE. No engine entry until its C checkpoint succeeds. */
- (int)requestActivationForPreparedWorker:(NgpPowerWorkerV1 *)worker;
/* Main only, nonblocking, terminal. App controller MUST call before hiding,
 * detaching or replacing source/window/scene. No automatic resume. */
- (int)revokeForLifecycle;
/* Main only. Genuine receipt plus terminal/idle reset/queued retirement.
 * Releases power owner only; controller retains source/host/code through G2. */
- (int)finishAfterWorkerJoin:(NgpPowerJoinReceiptV1 *)receipt;
@end
NS_ASSUME_NONNULL_END
#endif
#endif
