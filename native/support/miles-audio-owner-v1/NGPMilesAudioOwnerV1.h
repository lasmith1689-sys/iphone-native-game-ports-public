#import <UIKit/UIKit.h>
#include "audio_ticket.h"

NS_ASSUME_NONNULL_BEGIN

/* One process-retained owner, held by the retained game controller.
 * The controller calls begin only from the user's explicit playback action. */
@interface NGPMilesAudioOwnerV1 : NSObject
- (instancetype)init NS_UNAVAILABLE;
- (nullable instancetype)initWithScene:(UIWindowScene *)scene;
- (int)beginFromPlaybackActionForWorker:(NSThread *)worker;
- (int)copyReadyTicketOnWorker:(NgpMilesAudioTicketV1 *)output;
- (void)cancelOnMain;
@end

NS_ASSUME_NONNULL_END
