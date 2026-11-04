/* Own retained first-profile caller. Source candidate, not a ready game app. */
#ifndef NGP_RETAINED_GAME_CONTROLLER_V1_H
#define NGP_RETAINED_GAME_CONTROLLER_V1_H
#import <UIKit/UIKit.h>
#include "ngp_app_engine_frame_v1.h"
enum {NGP_BOOT_STAGE_SESSION=1,NGP_BOOT_STAGE_PREPARE=2,NGP_BOOT_STAGE_GL=3,
 NGP_BOOT_STAGE_ATTACH=4,NGP_BOOT_STAGE_POWER_PREPARE=5,NGP_BOOT_STAGE_POWER_BIND=6,
 NGP_BOOT_STAGE_ACTIVATION=7,NGP_BOOT_STAGE_WAIT=8,NGP_BOOT_STAGE_COMMON=9,
 NGP_BOOT_STAGE_FRAME=10,NGP_BOOT_STAGE_CANCEL=11,NGP_BOOT_STAGE_EXCEPTION=12};
enum {NGP_BOOT_CONTROLLER_ARGUMENT=-20101,NGP_BOOT_CONTROLLER_ALLOCATION=-20102,
 NGP_BOOT_CONTROLLER_CANCELLED=-20103,NGP_BOOT_CONTROLLER_EXCEPTION=-20104,
 NGP_BOOT_CONTROLLER_CLOCK=-20105,NGP_BOOT_CONTROLLER_PAUSE=-20106};
typedef struct {
 uint32_t version,bytes,stage,terminal;
 uint64_t sequence;int32_t status,snapshot_status;
 NgpAppEngineSnapshotV1 snapshot;NgpAppFrameReportV1 frame;
} NgpBootstrapObservationV1;

NS_ASSUME_NONNULL_BEGIN
__attribute__((objc_subclassing_restricted))
@interface NGPRetainedGameControllerV1 : UIViewController
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(nullable NSString *)name bundle:(nullable NSBundle *)bundle NS_UNAVAILABLE;
/* Main only; four exact image byte owners, printable profile including NUL,
 * ASCII command without NUL. Copies bytes into distinct immutable allocations.
 * Typed caller keeps input array/data readable and stable throughout the
 * synchronous copies; concurrent mutation during copy is not admitted.
 * Window must already belong to the actual single UIWindowScene. One attempt
 * per process; self/source/window/code/thread remain quarantined until exit. */
- (nullable instancetype)initWithWindow:(UIWindow *)window
 images:(NSArray<NSData *> *)images profileBytes:(NSData *)profile
 commandBytes:(NSData *)command commonBudget:(int64_t)commonBudget
 frameBudget:(int64_t)frameBudget;
/* Main after this controller is window.rootViewController, window visible and
 * actual scene active. One call only. Does not synchronously wait on engine. */
- (void)startInActiveScene;
/* Main MUST call before hide/detach/replacement/size change, terminal/no resume.
 * No join, free or retirement/G2 authority follows from this call. */
- (void)cancelBeforeLifecycleChange;
/* Main-only immutable copied POD facts, never a readiness token. */
@property(nonatomic,readonly,nullable) NSData *latestObservation;
@end
NS_ASSUME_NONNULL_END
#endif
