#import "NGPRetainedGameControllerV1.h"
#import "NGPAppSourceBindingV1.h"
#import "cod_gl_ios.h"
#include "ngp_app_gl_bind_v1.h"
#include "ngp_power_lease_v1.h"
#include "ngp_boot_wait_v1.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <math.h>

enum {NGP_LANE_BUSY=1,NGP_LANE_CANCEL=2};
static NGPRetainedGameControllerV1 *retained_controller;
static BOOL controller_claimed; /* main only, never reset */
static NSData *copy_bytes(NSData *bytes) {
 /* Typed caller keeps this source stable/readable throughout the synchronous
  * copy. NSData is not a sandbox against a concurrently mutating subclass. */
 NSUInteger count=bytes.length;const void *base=bytes.bytes;
 return [[NSData alloc] initWithBytes:base length:count];
}
static int monotonic_ns(uint64_t *out) {
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return -1;
 uint64_t sec=(uint64_t)t.tv_sec;
 if(sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/UINT64_C(1000000000))return -1;
 *out=sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;return 0;
}
@interface NGPRetainedGameControllerV1 () {
 UIWindow *_ownedWindow;UIWindowScene *_ownedScene;
 NSArray<NSData *> *_images;NSData *_profile,*_command;
 CodGLCompositionViewV1 *_drawable;CodInputSurfaceV3 *_source;
 NGPAppSourceBindingV1 *_binding;NgpPowerLeaseOwnerV1 *_powerOwner;
 CodGLHostCapsuleV1 *_capsule;NgpPowerLeaseV1 *_lease;
 NSThread *_engineThread;UILabel *_statusLabel;
 int64_t _commonBudget,_frameBudget;uint32_t _commandCount;uint64_t _incarnation;
 _Atomic(unsigned) _lane;
 BOOL _startAttempted,_terminalOnMain,_pauseAttempted;
 NSData *_latestObservation;uint64_t _displayedSequence;
 /* Worker-only values after NSThread start publication. Separate allocations,
  * never exposed as a guest pointer or freed on uncertain work. */
 NgpAppEngineSessionV1 *_session;NgpPowerWorkerV1 *_powerWorker;
 NgpAppFrameReportV1 *_frame;NgpAppEngineSnapshotV1 *_snapshot;
 uint64_t _statusSequence;
}
- (void)runEngine;
- (void)parkActualThread __attribute__((noreturn));
- (void)pauseIfIdleOnMain;
- (void)publishStage:(uint32_t)stage status:(int32_t)status terminal:(BOOL)terminal;
- (BOOL)beginLane;
- (void)endLane;
- (int)awaitActualPower;
@end

@implementation NGPRetainedGameControllerV1
- (instancetype)initWithWindow:(UIWindow *)window images:(NSArray<NSData *> *)images
 profileBytes:(NSData *)profile commandBytes:(NSData *)command
 commonBudget:(int64_t)commonBudget frameBudget:(int64_t)frameBudget {
 if(!NSThread.isMainThread||controller_claimed||!window.windowScene)return nil;
 controller_claimed=YES;
 self=[super initWithNibName:nil bundle:nil];if(!self)return nil;
 retained_controller=self; /* retain even partially initialized construction */
 atomic_init(&_lane,0);_ownedWindow=window;_ownedScene=window.windowScene;
 @try {
  if(!images||!profile||!command||commonBudget<=0||frameBudget<=0)return nil;
  NSArray<NSData *> *given=[images copy];if(given.count!=4)return nil;
  static const NSUInteger counts[4]={21393408,3063808,2224128,2572288};
  NSMutableArray<NSData *> *copies=[NSMutableArray arrayWithCapacity:4];
  for(NSUInteger i=0;i<4;i++){
   NSData *bytes=given[i];if(![bytes isKindOfClass:NSData.class])return nil;
   NSData *copy=copy_bytes(bytes);if(!copy||copy.length!=counts[i])return nil;[copies addObject:copy];
  }
  _images=[copies copy];_profile=copy_bytes(profile);NSData *commandCopy=copy_bytes(command);
  if(!_profile||_profile.length<2||_profile.length>96||!commandCopy||commandCopy.length>4095)return nil;
  const uint8_t *p=_profile.bytes;if(!p||p[_profile.length-1])return nil;
  for(NSUInteger i=0;i+1<_profile.length;i++)if(p[i]<0x20||p[i]>0x7e)return nil;
  /* Keep an actual readable command pointer even for the legal empty command. */
  _command=commandCopy.length?commandCopy:[NSData dataWithBytes:"" length:1];
  p=_command.bytes;if(!p)return nil;
  for(NSUInteger i=0;i<commandCopy.length;i++)if(p[i]<0x20||p[i]>0x7e)return nil;
  _commandCount=(uint32_t)commandCopy.length;_commonBudget=commonBudget;_frameBudget=frameBudget;
  _snapshot=calloc(1, sizeof(*_snapshot));_frame=calloc(1, sizeof(*_frame));
  if(!_images||!_profile||!_command||!_snapshot||!_frame)return nil;
  return self;
 }@catch(NSException *e){(void)e;return nil;}
}
- (void)loadView {
 UIView *root=[[UIView alloc] initWithFrame:_ownedWindow.bounds];root.backgroundColor=UIColor.blackColor;
 self.view=root;
 _statusLabel=[[UILabel alloc] initWithFrame:CGRectInset(root.bounds,20,20)];
 _statusLabel.textColor=UIColor.whiteColor;_statusLabel.numberOfLines=0;
 _statusLabel.text=@"Preparing native engine test";_statusLabel.userInteractionEnabled=NO;
 [root addSubview:_statusLabel];
}
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {return UIInterfaceOrientationMaskLandscapeRight;}
- (BOOL)shouldAutorotate {return NO;}
- (NSData *)latestObservation {return NSThread.isMainThread?_latestObservation:nil;}
- (void)startInActiveScene {
 if(!NSThread.isMainThread||_startAttempted)return;_startAttempted=YES;
 if(_terminalOnMain||_ownedWindow.rootViewController!=self||_ownedWindow.hidden||
    _ownedWindow.windowScene!=_ownedScene||_ownedScene.activationState!=UISceneActivationStateForegroundActive||
    UIApplication.sharedApplication.applicationState!=UIApplicationStateActive){[self cancelBeforeLifecycleChange];return;}
 @try {
  [self loadViewIfNeeded];[self.view layoutIfNeeded];
  CGRect bounds=self.view.bounds;
  if(self.view.window!=_ownedWindow||!isfinite(bounds.size.width)||!isfinite(bounds.size.height)||bounds.size.width<=0||bounds.size.height<=0){[self cancelBeforeLifecycleChange];return;}
  _drawable=[[CodGLCompositionViewV1 alloc] initWithFrame:bounds];
  _source=[[CodInputSurfaceV3 alloc] initInertWithFrame:bounds];
  if(!_drawable||!_source){[self cancelBeforeLifecycleChange];return;}
  _source.backgroundColor=UIColor.clearColor;
  [self.view insertSubview:_drawable atIndex:0];[self.view insertSubview:_source aboveSubview:_drawable];
  [_drawable layoutIfNeeded];
  _binding=[[NGPAppSourceBindingV1 alloc] initWithSource:_source];
  int gl=cod_gl_host_v1_create(_drawable,&_capsule);
  if(!_binding||gl||!_capsule){[self cancelBeforeLifecycleChange];return;}
  /* Independent power generation1 for the sole process owner, NOT input gen. */
  _incarnation=_source.incarnation;
  _powerOwner=[[NgpPowerLeaseOwnerV1 alloc] initWithSource:_source incarnation:_incarnation generation:1];
  _lease=[_powerOwner borrowedLease];
  if(!_powerOwner||!_lease){[self cancelBeforeLifecycleChange];return;}
  _engineThread=[[NSThread alloc] initWithTarget:self selector:@selector(runEngine) object:nil];
  _engineThread.name=@"Native CoD engine";_engineThread.stackSize=8u*1024u*1024u;
  if(!_engineThread){[self cancelBeforeLifecycleChange];return;}
  [_engineThread start];
 }@catch(NSException *e){(void)e;[self cancelBeforeLifecycleChange];}
}
- (void)cancelBeforeLifecycleChange {
 if(!NSThread.isMainThread)return;
 atomic_fetch_or_explicit(&_lane, NGP_LANE_CANCEL,memory_order_acq_rel);
 _terminalOnMain=YES;
 if(_lease)ngp_power_request_cancel_v1(_lease);
 if(_powerOwner)(void)[_powerOwner revokeForLifecycle];
 _statusLabel.text=@"Native engine test stopped. Relaunch to test again.";
 [self pauseIfIdleOnMain];
}
- (void)pauseIfIdleOnMain {
 if(!NSThread.isMainThread||_pauseAttempted||!_capsule)return;
 unsigned lane=atomic_load_explicit(&_lane,memory_order_acquire);
 if(!(lane&NGP_LANE_CANCEL)||(lane&NGP_LANE_BUSY))return;
 _pauseAttempted=YES;
 /* No new transaction can start after CANCEL. Busy/uncertain capsule refusal
  * retains everything; pause never implies close, join or G2 retirement. */
 (void)cod_gl_host_v1_pause(_capsule);
}
- (BOOL)beginLane {
 unsigned expected=0;
 return atomic_compare_exchange_strong_explicit(&_lane,&expected,NGP_LANE_BUSY,memory_order_acq_rel,memory_order_acquire);
}
- (void)endLane {
 unsigned old=atomic_fetch_and_explicit(&_lane,~(unsigned)NGP_LANE_BUSY,memory_order_acq_rel);
 if(old&NGP_LANE_CANCEL)dispatch_async(dispatch_get_main_queue(),^{[self pauseIfIdleOnMain];});
}
- (int)awaitActualPower {
 uint64_t now;NgpBootWaitV1 wait;
 if(monotonic_ns(&now)||ngp_boot_wait_begin_v1(&wait,now,UINT64_C(10000000000)))return NGP_BOOT_CONTROLLER_CLOCK;
 for(;;){
  int cancelled=(atomic_load_explicit(&_lane,memory_order_acquire)&NGP_LANE_CANCEL)!=0;
  int checkpoint=cancelled?-1:ngp_power_checkpoint_v1(_powerWorker);
  if(monotonic_ns(&now))return NGP_BOOT_CONTROLLER_CLOCK;
  int decision=ngp_boot_wait_poll_v1(&wait,now,cancelled,checkpoint);
  if(decision!=NGP_BOOT_WAIT_PENDING)return decision;
  struct timespec pause={0,2000000L};
  /* Scheduling pause, not a spin loop or a total-execution deadline theorem. */
  if(nanosleep(&pause,NULL)&&errno!=EINTR)return NGP_BOOT_CONTROLLER_PAUSE;
 }
}
- (void)publishStage:(uint32_t)stage status:(int32_t)status terminal:(BOOL)terminal {
 if(_statusSequence==UINT64_MAX)return;
 NgpBootstrapObservationV1 observation={0};
 observation.version=1;observation.bytes=sizeof(observation);observation.stage=stage;
 observation.status=status;observation.terminal=terminal;observation.sequence=++_statusSequence;
 observation.snapshot_status=_session?ngp_app_engine_snapshot_v1(_session,_snapshot):NGP_APP_FOREIGN;
 if(!observation.snapshot_status)observation.snapshot=*_snapshot;
 if(_frame)observation.frame=*_frame;
 NSData *owned=[NSData dataWithBytes:&observation length:sizeof(observation)];
 dispatch_async(dispatch_get_main_queue(),^{
  if(observation.sequence<=self->_displayedSequence)return;
  self->_displayedSequence=observation.sequence;self->_latestObservation=owned;
  if(observation.terminal){[self cancelBeforeLifecycleChange];
   self->_statusLabel.text=[NSString stringWithFormat:@"Engine test stopped at stage %u (status %d).",stage,status];
  }else if(!self->_terminalOnMain){
   self->_statusLabel.text=stage==NGP_BOOT_STAGE_FRAME?@"Engine frame call returned. Checking native integration.":@"Preparing native engine test";
  }
 });
}
- (void)runEngine {
 @autoreleasepool {
  uint32_t stage=NGP_BOOT_STAGE_SESSION;int32_t rc=0;
  @try {
   if(NSThread.isMainThread){rc=NGP_APP_MAIN;goto stopped;}
   if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_session_create_v1(&_session);[self endLane];if(rc)goto stopped;
   [self publishStage:stage status:rc terminal:NO];
   NgpAppImageV1 images[4];
   for(NSUInteger i=0;i<4;i++)images[i]=(NgpAppImageV1){_images[i].bytes,(uint32_t)_images[i].length};
   stage=NGP_BOOT_STAGE_PREPARE;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_prepare_v1(_session,_profile.bytes,images);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_GL;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=rt_gl_bind_native_v1(_capsule);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_ATTACH;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_attach_v1(_session,(__bridge void *)_binding);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_POWER_PREPARE;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_power_worker_prepare_v1(_lease,_incarnation,1,&_powerWorker);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_POWER_BIND;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_power_worker_bind_checkpoint_v1(_powerWorker);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_ACTIVATION;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=[_powerOwner requestActivationForPreparedWorker:_powerWorker];[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_WAIT;rc=[self awaitActualPower];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_COMMON;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_common_v1(_session,_powerWorker,_command.bytes,_commandCount,_commonBudget);
   [self endLane];if(rc)goto stopped;[self publishStage:stage status:rc terminal:NO];
   uint64_t lastPublish=0;
   for(;;){@autoreleasepool {
    stage=NGP_BOOT_STAGE_FRAME;if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
    rc=ngp_app_engine_frame_v1(_session,_powerWorker,_frameBudget,_frame);[self endLane];if(rc)goto stopped;
    uint64_t now;if(monotonic_ns(&now)){rc=NGP_BOOT_CONTROLLER_CLOCK;goto stopped;}
    if(!lastPublish||now-lastPublish>=UINT64_C(1000000000)){
     [self publishStage:stage status:rc terminal:NO];lastPublish=now;
    }
    struct timespec pause={0,1000000L};
    if(nanosleep(&pause,NULL)&&errno!=EINTR){rc=NGP_BOOT_CONTROLLER_PAUSE;goto stopped;}
   }}
stopped:
   atomic_fetch_or_explicit(&_lane,NGP_LANE_CANCEL,memory_order_acq_rel);
   ngp_power_request_cancel_v1(_lease);
   [self publishStage:stage status:rc terminal:YES];
  }@catch(NSException *e){
   (void)e;
   /* Escaped provider exception is uncertain completion. Preserve BUSY if set:
    * no main pause, close, retry or freeing follows from this handler. */
   atomic_fetch_or_explicit(&_lane,NGP_LANE_CANCEL,memory_order_acq_rel);
   ngp_power_request_cancel_v1(_lease);
   @try {[self publishStage:NGP_BOOT_STAGE_EXCEPTION status:NGP_BOOT_CONTROLLER_EXCEPTION terminal:YES];}
   @catch(NSException *again){(void)again;}
  }
  [self parkActualThread];
 }
}
- (void)parkActualThread {
 /* Keep the ACTUAL engine pthread alive; retained NSThread alone is not enough.
  * There is no join/destruction/G2 or automatic resume in this first profile. */
 for(;;){struct timespec pause={3600,0};(void)nanosleep(&pause,NULL);}
}
- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
 [self cancelBeforeLifecycleChange];[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}
@end
