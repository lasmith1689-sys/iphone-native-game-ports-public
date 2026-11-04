/* PRIVATE own main-created binding capsule, not the full app controller. */
#ifndef NGP_APP_SOURCE_BINDING_V1_H
#define NGP_APP_SOURCE_BINDING_V1_H
#import <Foundation/Foundation.h>
#import "cod_uikit_input_v3.h"
NS_ASSUME_NONNULL_BEGIN
__attribute__((objc_subclassing_restricted))
@interface NGPAppSourceBindingV1 : NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
/* Main only, same exact real source. Creates/retains actual foreground using
 * that source's actual service.generation. Caller strongly retains capsule,
 * source, code and future main controller through unresolved host/input G2. */
- (nullable instancetype)initWithSource:(CodInputSurfaceV3 *)source NS_DESIGNATED_INITIALIZER;
@end
NS_ASSUME_NONNULL_END
#endif
