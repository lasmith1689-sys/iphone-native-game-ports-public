/* Own retained first-profile caller. Source candidate, not a ready game app. */
#ifndef NGP_RETAINED_GAME_CONTROLLER_V2_H
#define NGP_RETAINED_GAME_CONTROLLER_V2_H
#import <UIKit/UIKit.h>
#include "ngp_app_engine_frame_v1.h"
#include "ngp_resource_loader_v1.h"
#include "ngp_boot_failure_v1.h"
enum {NGP_BOOT_STAGE_SESSION=1,NGP_BOOT_STAGE_PREPARE=2,NGP_BOOT_STAGE_GL=3,
 NGP_BOOT_STAGE_ATTACH=4,NGP_BOOT_STAGE_POWER_PREPARE=5,NGP_BOOT_STAGE_POWER_BIND=6,
 NGP_BOOT_STAGE_ACTIVATION=7,NGP_BOOT_STAGE_WAIT=8,NGP_BOOT_STAGE_COMMON=9,
 NGP_BOOT_STAGE_FRAME=10,NGP_BOOT_STAGE_CANCEL=11,NGP_BOOT_STAGE_EXCEPTION=12,
 NGP_BOOT_STAGE_RESOURCE_OPEN=13,NGP_BOOT_STAGE_RESOURCE_CREATE=14,
 NGP_BOOT_STAGE_RESOURCE_VALIDATE=15,NGP_BOOT_STAGE_RESOURCE_CWD=16,
 NGP_BOOT_STAGE_RESOURCE_BORROW=17,NGP_BOOT_STAGE_SETUP_VIEW=18,
 NGP_BOOT_STAGE_SETUP_SOURCE=19,NGP_BOOT_STAGE_SETUP_BINDING=20,
 NGP_BOOT_STAGE_SETUP_GL=21,NGP_BOOT_STAGE_SETUP_POWER=22,NGP_BOOT_STAGE_SETUP_THREAD=23};
enum {NGP_BOOT_CONTROLLER_ARGUMENT=-20101,NGP_BOOT_CONTROLLER_ALLOCATION=-20102,
 NGP_BOOT_CONTROLLER_CANCELLED=-20103,NGP_BOOT_CONTROLLER_EXCEPTION=-20104,
 NGP_BOOT_CONTROLLER_CLOCK=-20105,NGP_BOOT_CONTROLLER_PAUSE=-20106,
 NGP_BOOT_CONTROLLER_CONSTRUCTION=-20107};
typedef struct {
 uint32_t version,bytes,stage,terminal;
 uint64_t sequence;int32_t status,snapshot_status;
 NgpAppEngineSnapshotV1 snapshot;NgpAppFrameReportV1 frame;
 NgpResourceReportV1 resources;
 uint32_t first_failure_present;NgpBootFailureV1 first_failure;
} NgpBootstrapObservationV1;

NS_ASSUME_NONNULL_BEGIN
__attribute__((objc_subclassing_restricted))
@interface NGPRetainedGameControllerV2 : UIViewController
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(nullable NSString *)name bundle:(nullable NSBundle *)bundle NS_UNAVAILABLE;
/* Main only; completed private image/asset directories, profile including NUL,
 * ASCII command without NUL. Copies bytes into distinct immutable allocations.
 * Typed caller keeps input data readable and stable throughout the
 * synchronous copies; concurrent mutation during copy is not admitted.
 * Roots and complete ancestors are trusted/immutable after completed private
 * import. No concurrent AFC/Files/iCloud writes or other process cwd owner.
 * The fixed resource loader checks its real namespaces/hashes before borrowing.
 * Window must already belong to the actual single UIWindowScene. One attempt
 * per process; self/source/window/code/thread remain quarantined until exit. */
- (nullable instancetype)initWithWindow:(UIWindow *)window
 imageDirectoryURL:(NSURL *)images assetDirectoryURL:(NSURL *)assets profileBytes:(NSData *)profile
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
/* Main-only allocation-free first-cause copy, independent of diagnostic
 * delivery. Output must be live correctly sized disjoint native storage.
 * NO leaves output untouched; this grants no quiescence/engine authority. */
- (BOOL)copyFirstFailure:(NgpBootFailureV1 *)out;
@end
NS_ASSUME_NONNULL_END
#endif
