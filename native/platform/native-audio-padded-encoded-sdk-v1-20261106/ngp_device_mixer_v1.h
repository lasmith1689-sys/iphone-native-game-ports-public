#ifndef NGP_DEVICE_MIXER_V1_H
#define NGP_DEVICE_MIXER_V1_H
#include "ngp_voice_service_v1.h"
enum {NGP_DEVICE_BROKER_OK_V1=0,NGP_DEVICE_BROKER_ARGUMENT_V1=1,
 NGP_DEVICE_BROKER_INTENT_V1=2,NGP_DEVICE_BROKER_ALLOC_V1=3,
 NGP_DEVICE_BROKER_ROUTE_V1=4,NGP_DEVICE_BROKER_SESSION_V1=5,
 NGP_DEVICE_BROKER_GRAPH_V1=6,NGP_DEVICE_BROKER_EXCEPTION_V1=7,
 NGP_DEVICE_BROKER_CLOSED_V1=8,NGP_DEVICE_BROKER_PENDING_V1=9};
enum {NGP_DEVICE_BROKER_OPENING_V1=1,NGP_DEVICE_BROKER_ACTIVE_V1=2,
 NGP_DEVICE_BROKER_FAILED_RETAINED_V1=3,NGP_DEVICE_BROKER_STOPPED_RETAINED_V1=4};
enum {NGP_DEVICE_STOP_PENDING_V1=0,NGP_DEVICE_STOP_ACK_V1=1,NGP_DEVICE_STOP_FAILED_V1=2};
/* Historical retained identity and monotone outcome, independent of ACTIVE,
 * current intent, mutable broker phase and queue-owned ObjC properties. */
typedef struct {
 uint32_t version,size,outcome,reserved;
 uint64_t intent_epoch;
 NgpVoiceServiceV1 *voice_service;
} NgpDeviceMixerStopObservationV1;
typedef struct {
 uint32_t version,size,phase,sample_rate,channels,maximum_frames;
 uint64_t intent_epoch;
 NgpVoiceServiceV1 *voice_service;
} NgpDeviceMixerFactsV1;
/* Real canonical factory/queue admission; no caller ready flag. C wrapper
 * catches actual ObjC exceptions and returns before any plain-C nonreturn. */
int ngp_device_mixer_wait_admit_v1(void*,NgpVoiceServiceV1*,uint64_t);
#ifdef __OBJC__
#import <UIKit/UIKit.h>
/* Actual main scene owner. App calls requestPlaybackIntent only from its user
 * playback action. Scene activation alone never grants that intent. */
@interface NGPDeviceMixerIntentV1:NSObject
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithScene:(UIWindowScene*)scene;
- (uint64_t)requestPlaybackIntent;
- (void)clearPlaybackIntent;
@end
/* Sole app-wide device broker; do not link the older finite queued-player
 * broker. All in-app device users must join this broker. One process attempt;
 * no restart/destroy, callback quiescence, signing or physical-readiness claim. */
@interface NGPDeviceMixerBrokerV1:NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)openFromIntent:(NGPDeviceMixerIntentV1*)intent;
/* Poll from the persistent engine worker BEFORE guest entry. Success reports
 * actual device activation only. Caller then prepares/enables the real voice
 * service separately. Output starts version1/size and is unchanged on failure. */
- (int)copyDeviceFacts:(NgpDeviceMixerFactsV1*)facts;
/* Any-thread request: atomically revoke first, enqueue stop; never a free/G2. */
- (void)requestStop;
/* Real one-shot enqueue status, never a stop acknowledgement. */
- (int)requestStopTracked;
/* Read-only actual capsule/atomic observation, output unchanged on error.
 * Native object must be valid and canonically retained/published by the app. */
- (int)copyStopObservation:(NgpDeviceMixerStopObservationV1*)observation;
@end
#endif
#endif
