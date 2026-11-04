#ifndef COD_UIKIT_INPUT_V3_H
#define COD_UIKIT_INPUT_V3_H
#import <UIKit/UIKit.h>
#include "cod_input_queue_v2.h"
enum {COD_BIND_INERT_V3=0,COD_BIND_QUEUED_V3=1,COD_BIND_INSTALLED_V3=2,
 COD_BIND_BOUND_V3=3,COD_BIND_ABANDONED_V3=4};
typedef struct {uint32_t version,size,phase,attempted,pending,uncertain;
 uint64_t incarnation,issued,retired;} CodInputBindingStatusV3;
NS_ASSUME_NONNULL_BEGIN
/* OWN source only. App retains it and all host/table owners through G2, which
 * is not implemented here. No host/code release follows from pending0. */
@interface CodInputSurfaceV3:UIView
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
/* Main-only real inert source. No descriptor, events, ready/start or generation. */
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (nullable instancetype)initInertWithFrame:(CGRect)frame NS_DESIGNATED_INITIALIZER;
@property(nonatomic,readonly) uint64_t incarnation;
/* One serialized TRUE engine pthread; after v18 prepare has returned/released
 * host serial, pass its actual stable rt_input_consumer_v1 result. Caller owns
 * readable table/context lifetime, holds no host/engine/source lock, keeps main
 * responsive. This API validates structural/table identity, not provenance of
 * arbitrary C pointers. Main/invalid calls also consume the sole attempt.
 * Requested500ms wait is not a total deadline. Timeout is terminal; a late
 * main block never commits. A return0 binds, never starts input. */
- (int32_t)bindActualEngine:(const CodInputEngineV1*)actual incarnation:(uint64_t)incarnation;
/* Locked own request facts only, no whole-provider/host-drain authority. */
- (CodInputBindingStatusV3)bindingStatus;
@end
CodNativeInputV1 cod_uikit_input_v3_service(void* _Nullable retained_surface);
NS_ASSUME_NONNULL_END
#endif
