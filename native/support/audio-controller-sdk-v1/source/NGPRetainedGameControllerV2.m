#import "NGPRetainedGameControllerV2.h"
#import "NGPAppSourceBindingV1.h"
#import "cod_gl_ios.h"
#include "ngp_app_gl_bind_v1.h"
/* Captured power API mixes unannotated C declarations and annotated ObjC.
 * Preserve its exact ABI/header; scope only this existing completeness warning. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include "ngp_power_lease_v1.h"
#pragma clang diagnostic pop
#include "ngp_boot_wait_v1.h"
#include "ngp_app_miles_binding_v1.h"
#import "NGPMilesAudioOwnerV1.h"
enum {NGP_BOOT_STAGE_AUDIO_INTENT=24,NGP_BOOT_STAGE_AUDIO_WAIT=25,NGP_BOOT_STAGE_AUDIO_BIND=26};
#import "ngp_boot_trace_objc_v1.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

enum {NGP_LANE_BUSY=1,NGP_LANE_CANCEL=2};
static NGPRetainedGameControllerV2 *retained_controller;
static BOOL controller_claimed; /* main only, never reset */
static NSData *copy_bytes(NSData *bytes) {
 /* Typed caller keeps this source stable/readable throughout the synchronous
  * copy. NSData is not a sandbox against a concurrently mutating subclass. */
 NSUInteger count=bytes.length;const void *base=bytes.bytes;
 return [[NSData alloc] initWithBytes:base length:count];
}
static NSData *copy_directory_path(NSURL *url) {
 if(!url.isFileURL)return nil;const char *path=url.fileSystemRepresentation;
 if(!path)return nil;size_t n=strnlen(path,4096);if(!n||n>=4096||path[0]!='/')return nil;
 for(size_t i=0;i<n;i++)if((uint8_t)path[i]<0x20||(uint8_t)path[i]>0x7e)return nil;
 return [NSData dataWithBytes:path length:n+1];
}
static int monotonic_ns(uint64_t *out) {
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000L)return -1;
 uint64_t sec=(uint64_t)t.tv_sec;
 if(sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/UINT64_C(1000000000))return -1;
 *out=sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;return 0;
}
@interface NGPRetainedGameControllerV2 () {
 UIWindow *_ownedWindow;UIWindowScene *_ownedScene;
 NSData *_imageRootPath,*_assetRootPath,*_profile,*_command;
 CodGLCompositionViewV1 *_drawable;CodInputSurfaceV3 *_source;
 NGPAppSourceBindingV1 *_binding;NgpPowerLeaseOwnerV1 *_powerOwner;
 CodGLHostCapsuleV1 *_capsule;NgpPowerLeaseV1 *_lease;
 NSThread *_engineThread;UILabel *_statusLabel;
 UIButton *_startPlaybackButton;
 NGPMilesAudioOwnerV1 *_audioOwner;
 NgpMilesAudioTicketV1 _audioTicket; /* worker only */
 int64_t _commonBudget,_frameBudget;uint32_t _commandCount;uint64_t _incarnation;
 _Atomic(unsigned) _lane;
 NgpBootFailureLatchV1 _firstFailure;
 BOOL _startAttempted,_terminalOnMain,_pauseAttempted,_pauseQueued;
 NSData *_latestObservation;uint64_t _displayedSequence;
 NSLock *_observationLock;NSData *_pendingObservation;BOOL _observationQueued;
 _Atomic(NgpResourceSetV1 *) _resources;
 NgpResourceReportV1 *_resourceReport;uint32_t _resourceStage;
 int _imageRootFD,_assetRootFD;
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
- (void)consumeObservationOnMain;
- (void)recordSetupFailure:(uint32_t)stage status:(int32_t)status;
- (BOOL)beginLane;
- (void)endLane;
- (int)awaitActualPower;
- (int)awaitActualAudio;
- (void)beginPlayback:(UIButton *)sender;
- (int32_t)prepareActualResources:(NgpAppImageV1 *)images;
@end

@implementation NGPRetainedGameControllerV2
- (instancetype)initWithWindow:(UIWindow *)window imageDirectoryURL:(NSURL *)images assetDirectoryURL:(NSURL *)assets
 profileBytes:(NSData *)profile commandBytes:(NSData *)command
 commonBudget:(int64_t)commonBudget frameBudget:(int64_t)frameBudget {
 if(!NSThread.isMainThread||controller_claimed||!window.windowScene)return nil;
 controller_claimed=YES;
 self=[super initWithNibName:nil bundle:nil];if(!self)return nil;
 retained_controller=self; /* retain even partially initialized construction */
 atomic_init(&_lane,0);atomic_init(&_resources,NULL);ngp_boot_failure_init_v1(&_firstFailure);
 _imageRootFD=-1;_assetRootFD=-1;_ownedWindow=window;_ownedScene=window.windowScene;
 @try {
  if(!images||!assets||!profile||!command||commonBudget<=0||frameBudget<=0)return nil;
  _imageRootPath=copy_directory_path(images);_assetRootPath=copy_directory_path(assets);
  _profile=copy_bytes(profile);NSData *commandCopy=copy_bytes(command);
  if(!_profile||_profile.length<2||_profile.length>96||!commandCopy||commandCopy.length>4095)return nil;
  const uint8_t *p=_profile.bytes;if(!p||p[_profile.length-1])return nil;
  for(NSUInteger i=0;i+1<_profile.length;i++)if(p[i]<0x20||p[i]>0x7e)return nil;
  /* Keep an actual readable command pointer even for the legal empty command. */
  _command=commandCopy.length?commandCopy:[NSData dataWithBytes:"" length:1];
  p=_command.bytes;if(!p)return nil;
  for(NSUInteger i=0;i<commandCopy.length;i++)if(p[i]<0x20||p[i]>0x7e)return nil;
  _commandCount=(uint32_t)commandCopy.length;_commonBudget=commonBudget;_frameBudget=frameBudget;
  _snapshot=calloc(1, sizeof(*_snapshot));_frame=calloc(1, sizeof(*_frame));
  _resourceReport=calloc(1,sizeof(*_resourceReport));
  _observationLock=[[NSLock alloc] init];
  if(!_imageRootPath||!_assetRootPath||!_profile||!_command||!_snapshot||!_frame||!_resourceReport||!_observationLock)return nil;
  _resourceReport->version=1;_resourceReport->size=sizeof(*_resourceReport);
  return self;
 }@catch(NSException *e){(void)e;return nil;}
}
- (void)loadView {
 NGPBT(LOADVIEW_ENTER,0,0,0,0);
 UIView *root=[[UIView alloc] initWithFrame:_ownedWindow.bounds];root.backgroundColor=UIColor.blackColor;
 self.view=root;
 _statusLabel=[[UILabel alloc] initWithFrame:CGRectInset(root.bounds,20,20)];
 _statusLabel.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
 _statusLabel.textColor=UIColor.whiteColor;_statusLabel.numberOfLines=0;
 _statusLabel.text=@"Preparing native engine test";_statusLabel.userInteractionEnabled=NO;
 [root addSubview:_statusLabel];
 ngpbt_view_geometry(NGP_BT_LOADVIEW_EXIT,root,_statusLabel);
}
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {return UIInterfaceOrientationMaskLandscapeRight;}
- (BOOL)shouldAutorotate {return NO;}
- (NSData *)latestObservation {return NSThread.isMainThread?_latestObservation:nil;}
- (BOOL)copyFirstFailure:(NgpBootFailureV1 *)out {
 if(!NSThread.isMainThread)return NO;
 return ngp_boot_failure_copy_v1(&_firstFailure,out)?YES:NO;
}
- (void)startInActiveScene {
 if(!NSThread.isMainThread)return;
 NGPBT(START_ENTER,_startAttempted,_terminalOnMain,0,0);
 if(_startAttempted){if(_terminalOnMain)[self cancelBeforeLifecycleChange];return;}_startAttempted=YES;
 ngpbt_start_gate(_ownedWindow,_ownedScene,self);
 if(_terminalOnMain||_ownedWindow.rootViewController!=self||_ownedWindow.hidden||
    _ownedWindow.windowScene!=_ownedScene||_ownedScene.activationState!=UISceneActivationStateForegroundActive||
    UIApplication.sharedApplication.applicationState!=UIApplicationStateActive){[self recordSetupFailure:NGP_BOOT_STAGE_SETUP_VIEW status:NGP_BOOT_CONTROLLER_ARGUMENT];return;}
 uint32_t setupStage=NGP_BOOT_STAGE_SETUP_VIEW;
 @try {
  [self loadViewIfNeeded];[self.view layoutIfNeeded];
  CGRect bounds=self.view.bounds;
  if(self.view.window!=_ownedWindow||!isfinite(bounds.size.width)||!isfinite(bounds.size.height)||bounds.size.width<=0||bounds.size.height<=0){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_ARGUMENT];return;}
  setupStage=NGP_BOOT_STAGE_SETUP_SOURCE;
  NGPBT(SETUP_STAGE,setupStage,ngpbt_pt(bounds.size.width),ngpbt_pt(bounds.size.height),0);
  _drawable=[[CodGLCompositionViewV1 alloc] initWithFrame:bounds];
  _source=[[CodInputSurfaceV3 alloc] initInertWithFrame:bounds];
  if(!_drawable||!_source){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  _source.backgroundColor=UIColor.clearColor;
  [self.view insertSubview:_drawable atIndex:0];[self.view insertSubview:_source aboveSubview:_drawable];
  [_drawable layoutIfNeeded];
  setupStage=NGP_BOOT_STAGE_SETUP_BINDING;
  NGPBT(SETUP_STAGE,setupStage,0,0,0);
  _binding=[[NGPAppSourceBindingV1 alloc] initWithSource:_source];
  if(!_binding){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  setupStage=NGP_BOOT_STAGE_SETUP_GL;
  NGPBT(SETUP_STAGE,setupStage,0,0,0);
  NGPBT(GL_CREATE_ENTER,0,0,0,0);
  int gl=cod_gl_host_v1_create(_drawable,&_capsule);
  NGPBT(GL_CREATE_EXIT,gl,_capsule!=NULL,0,0);
  if(gl||!_capsule){[self recordSetupFailure:setupStage status:gl?gl:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  /* Independent power generation1 for the sole process owner, NOT input gen. */
  _incarnation=_source.incarnation;
  setupStage=NGP_BOOT_STAGE_SETUP_POWER;
  NGPBT(SETUP_STAGE,setupStage,0,0,0);
  _powerOwner=[[NgpPowerLeaseOwnerV1 alloc] initWithSource:_source incarnation:_incarnation generation:1];
  _lease=[_powerOwner borrowedLease];
  if(!_powerOwner||!_lease){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  setupStage=NGP_BOOT_STAGE_SETUP_THREAD;
  NGPBT(SETUP_STAGE,setupStage,0,0,0);
  _engineThread=[[NSThread alloc] initWithTarget:self selector:@selector(runEngine) object:nil];
  _engineThread.name=@"Native CoD engine";_engineThread.stackSize=8u*1024u*1024u;
  if(!_engineThread){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  setupStage=NGP_BOOT_STAGE_AUDIO_INTENT;
  _audioOwner=[[NGPMilesAudioOwnerV1 alloc] initWithScene:_ownedScene];
  _startPlaybackButton=[UIButton buttonWithType:UIButtonTypeSystem];
  if(!_audioOwner||!_startPlaybackButton){[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_CONSTRUCTION];return;}
  _startPlaybackButton.frame=CGRectMake(CGRectGetMidX(bounds)-110,CGRectGetMidY(bounds)+28,220,52);
  _startPlaybackButton.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|
   UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleBottomMargin;
  [_startPlaybackButton setTitle:@"Start native test" forState:UIControlStateNormal];
  [_startPlaybackButton addTarget:self action:@selector(beginPlayback:) forControlEvents:UIControlEventTouchUpInside];
  _statusLabel.text=@"Ready to prepare the engine";
  [self.view addSubview:_startPlaybackButton];
 }@catch(NSException *e){ngpbt_exception(NGP_BT_START_EXCEPTION,e);(void)e;[self recordSetupFailure:setupStage status:NGP_BOOT_CONTROLLER_EXCEPTION];}
}
- (void)beginPlayback:(UIButton *)sender {
 if(!NSThread.isMainThread||_terminalOnMain||!sender||sender!=_startPlaybackButton||!sender.enabled)return;
 sender.enabled=NO;
 @try {
  if(_ownedWindow.rootViewController!=self||_ownedWindow.hidden||
     _ownedScene.activationState!=UISceneActivationStateForegroundActive||
     UIApplication.sharedApplication.applicationState!=UIApplicationStateActive){
   [self recordSetupFailure:NGP_BOOT_STAGE_AUDIO_INTENT status:NGP_BOOT_CONTROLLER_ARGUMENT];return;
  }
  int rc=[_audioOwner beginFromPlaybackActionForWorker:_engineThread];
  if(rc){[self recordSetupFailure:NGP_BOOT_STAGE_AUDIO_INTENT status:rc];return;}
  [_startPlaybackButton removeFromSuperview];
  _statusLabel.text=@"Checking local game files and preparing engine";
  NGPBT(THREAD_START_ENTER,0,0,0,0);
  [_engineThread start];
  NGPBT(THREAD_START_EXIT,0,0,0,0);
 }@catch(NSException *exception){
  ngpbt_exception(NGP_BT_START_EXCEPTION,exception);
  [self recordSetupFailure:NGP_BOOT_STAGE_AUDIO_INTENT status:NGP_BOOT_CONTROLLER_EXCEPTION];
 }
}
- (void)recordSetupFailure:(uint32_t)stage status:(int32_t)status {
 if(!NSThread.isMainThread)return;
 NGPBT(SETUP_FAILURE,stage,status,0,0);
 (void)ngp_boot_failure_record_v1(&_firstFailure,stage,status);
 /* This can also run after an uncertain NSThread start. Do not read the
  * worker's mutable report/session/sequence fields from this main path. */
 @try {
  NgpBootstrapObservationV1 observation={0};observation.version=1;observation.bytes=sizeof(observation);
  observation.stage=stage;observation.status=status;observation.terminal=1;observation.snapshot_status=NGP_APP_MAIN;
  observation.first_failure_present=ngp_boot_failure_copy_v1(&_firstFailure,&observation.first_failure);
  _latestObservation=[NSData dataWithBytes:&observation length:sizeof(observation)];
 }@catch(NSException *reporting){(void)reporting;/* cause remains published independently */}
 [self cancelBeforeLifecycleChange];
}
- (void)cancelBeforeLifecycleChange {
 if(!NSThread.isMainThread)return;
 NGPBT(CANCEL,atomic_load_explicit(&_lane,memory_order_acquire),_terminalOnMain,0,0);
 (void)ngp_boot_failure_record_v1(&_firstFailure,NGP_BOOT_STAGE_CANCEL,NGP_BOOT_CONTROLLER_CANCELLED);
 atomic_fetch_or_explicit(&_lane, NGP_LANE_CANCEL,memory_order_acq_rel);
 _terminalOnMain=YES;
 _startPlaybackButton.enabled=NO;
 [_audioOwner cancelOnMain];
 if(_lease)ngp_power_request_cancel_v1(_lease);
 NgpResourceSetV1 *resources=atomic_load_explicit(&_resources,memory_order_acquire);
 if(resources)(void)ngp_resources_cancel_v1(resources);
 if(_powerOwner)(void)[_powerOwner revokeForLifecycle];
 _statusLabel.text=@"Native engine test stopped. Relaunch to test again.";
 if(!_pauseQueued){_pauseQueued=YES;dispatch_async(dispatch_get_main_queue(),^{
  self->_pauseQueued=NO;[self pauseIfIdleOnMain];
 });}
}
- (void)pauseIfIdleOnMain {
 if(!NSThread.isMainThread||_pauseAttempted||!_capsule)return;
 unsigned lane=atomic_load_explicit(&_lane,memory_order_acquire);
 if(!(lane&NGP_LANE_CANCEL)||(lane&NGP_LANE_BUSY))return;
 /* Never issue deferred GL finish/pause while inactive/backgrounded. Retain
  * uncertain context; a later active scene may settle it, never resume engine. */
 if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive||
    _ownedScene.activationState!=UISceneActivationStateForegroundActive)return;
 _pauseAttempted=YES;
 /* No new transaction can start after CANCEL. Busy/uncertain capsule refusal
  * retains everything; pause never implies close, join or G2 retirement. */
 NGPBT(PAUSE_ENTER,0,0,0,0);
 (void)cod_gl_host_v1_pause(_capsule);
 NGPBT(PAUSE_EXIT,0,0,0,0);
}
- (int32_t)prepareActualResources:(NgpAppImageV1 *)images {
 _resourceStage=NGP_BOOT_STAGE_RESOURCE_OPEN;
 NGPBT(WORKER_STAGE,_resourceStage,0,0,0);
 if(![self beginLane])return NGP_BOOT_CONTROLLER_CANCELLED;
 /* Only these two private completed roots; parent/ancestor immutability is the
  * selected app/import contract. Root FD and all uncertain ownership persist. */
 _imageRootFD=open(_imageRootPath.bytes,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
 int open_error=_imageRootFD<0?errno:0;
 _assetRootFD=open(_assetRootPath.bytes,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
 if(_assetRootFD<0&&!open_error)open_error=errno;
 int32_t rc=(_imageRootFD<0||_assetRootFD<0)?-NGPR_IO:0;
 if(rc){_resourceReport->status=NGPR_IO;_resourceReport->native_error=open_error;
  _resourceReport->stage=NGPR_STAGE_CREATE;_resourceReport->file_index=UINT32_MAX;}
 [self endLane];if(rc)return rc;
 _resourceStage=NGP_BOOT_STAGE_RESOURCE_CREATE;
 NGPBT(WORKER_STAGE,_resourceStage,0,0,0);
 if(![self beginLane])return NGP_BOOT_CONTROLLER_CANCELLED;
 NgpResourceRootsV1 roots={1,sizeof(roots),_imageRootFD,_assetRootFD,_profile.bytes};
 NgpResourceSetV1 *resources=NULL;
 rc=ngp_resources_create_v1(&roots,&resources,_resourceReport);
 atomic_store_explicit(&_resources,resources,memory_order_release);
 [self endLane];if(rc)return rc;
 _resourceStage=NGP_BOOT_STAGE_RESOURCE_VALIDATE;
 NGPBT(WORKER_STAGE,_resourceStage,0,0,0);
 if(![self beginLane])return NGP_BOOT_CONTROLLER_CANCELLED;
 rc=ngp_resources_validate_v1(resources,_resourceReport);[self endLane];if(rc)return rc;
 _resourceStage=NGP_BOOT_STAGE_RESOURCE_CWD;
 NGPBT(WORKER_STAGE,_resourceStage,0,0,0);
 if(![self beginLane])return NGP_BOOT_CONTROLLER_CANCELLED;
 rc=ngp_resources_enter_cwd_v1(resources,_resourceReport);[self endLane];if(rc)return rc;
 _resourceStage=NGP_BOOT_STAGE_RESOURCE_BORROW;
 NGPBT(WORKER_STAGE,_resourceStage,0,0,0);
 if(![self beginLane])return NGP_BOOT_CONTROLLER_CANCELLED;
 NgpResourceImageViewV1 views[4]={{0}};
 rc=ngp_resources_borrow_for_prepare_v1(resources,views,_resourceReport);
 if(!rc)for(unsigned i=0;i<4;i++){
  images[i].bytes=views[i].bytes;images[i].count=views[i].count;
 }
 [self endLane];return rc;
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
- (int)awaitActualAudio {
 uint64_t now;NgpBootWaitV1 wait;
 if(monotonic_ns(&now)||ngp_boot_wait_begin_v1(&wait,now,UINT64_C(10000000000)))return NGP_BOOT_CONTROLLER_CLOCK;
 for(;;){
  int cancelled=(atomic_load_explicit(&_lane,memory_order_acquire)&NGP_LANE_CANCEL)!=0;
  int checkpoint=cancelled?NGP_AUDIO_WORKER_RETAINED:[_audioOwner copyReadyTicketOnWorker:&_audioTicket];
  if(!cancelled&&checkpoint&&checkpoint!=NGP_AUDIO_WORKER_PENDING)return checkpoint;
  if(monotonic_ns(&now))return NGP_BOOT_CONTROLLER_CLOCK;
  int decision=ngp_boot_wait_poll_v1(&wait,now,cancelled,checkpoint);
  if(decision!=NGP_BOOT_WAIT_PENDING)return decision;
  struct timespec pause={0,2000000L};
  if(nanosleep(&pause,NULL)&&errno!=EINTR)return NGP_BOOT_CONTROLLER_PAUSE;
 }
}
- (void)publishStage:(uint32_t)stage status:(int32_t)status terminal:(BOOL)terminal {
 NGPBT(PUBLISH,stage,status,terminal,_statusSequence);
 if(terminal)(void)ngp_boot_failure_record_v1(&_firstFailure,stage,status);
 if(_statusSequence==UINT64_MAX)return;
 NgpBootstrapObservationV1 observation={0};
 observation.version=1;observation.bytes=sizeof(observation);observation.stage=stage;
 observation.status=status;observation.terminal=terminal;observation.sequence=++_statusSequence;
 observation.first_failure_present=ngp_boot_failure_copy_v1(&_firstFailure,&observation.first_failure);
 observation.snapshot_status=_session?ngp_app_engine_snapshot_v1(_session,_snapshot):NGP_APP_FOREIGN;
 if(!observation.snapshot_status)observation.snapshot=*_snapshot;
 if(_frame)observation.frame=*_frame;
 if(_resourceReport)observation.resources=*_resourceReport;
 NSData *owned=[NSData dataWithBytes:&observation length:sizeof(observation)];
 BOOL enqueue=NO;[_observationLock lock];
 @try {_pendingObservation=owned;enqueue=!_observationQueued;
  if(enqueue)_observationQueued=YES;
 }@finally {[_observationLock unlock];}
 if(enqueue)dispatch_async(dispatch_get_main_queue(),^{[self consumeObservationOnMain];});
}
- (void)consumeObservationOnMain {
 if(!NSThread.isMainThread)return;
 NSData *owned;[_observationLock lock];
 @try {owned=_pendingObservation;_pendingObservation=nil;_observationQueued=NO;}
 @finally {[_observationLock unlock];}
 if(owned.length!=sizeof(NgpBootstrapObservationV1)){[self cancelBeforeLifecycleChange];return;}
 NgpBootstrapObservationV1 observation;[owned getBytes:&observation length:sizeof(observation)];
 if(observation.sequence<=_displayedSequence)return;
 /* A queued pre-failure observation cannot hide a subsequently published
  * immutable cause. The independent POD accessor also works if this copy fails. */
 observation.first_failure_present=ngp_boot_failure_copy_v1(&_firstFailure,&observation.first_failure);
 if(_terminalOnMain)observation.terminal=1;
 _displayedSequence=observation.sequence;
 NGPBT(CONSUME,observation.stage,observation.status,observation.terminal,observation.sequence);
 _latestObservation=[NSData dataWithBytes:&observation length:sizeof(observation)];
 if(observation.terminal){[self cancelBeforeLifecycleChange];
  uint32_t stage=observation.first_failure_present?observation.first_failure.stage:observation.stage;
  int32_t status=observation.first_failure_present?observation.first_failure.status:observation.status;
  _statusLabel.text=[NSString stringWithFormat:@"Engine test stopped at stage %u (status %d).",stage,status];
 }else if(!_terminalOnMain){
  _statusLabel.text=observation.stage==NGP_BOOT_STAGE_FRAME?@"Engine frame call returned. Checking native integration.":@"Checking local game files and preparing engine";
 }
}
- (void)runEngine {
 @autoreleasepool {
  NGPBT(WORKER_ENTER,0,0,0,0);
  uint32_t stage=NGP_BOOT_STAGE_SESSION;int32_t rc=0;
  @try {
   if(NSThread.isMainThread){rc=NGP_APP_MAIN;goto stopped;}
   NgpAppImageV1 images[4]={{0}};stage=NGP_BOOT_STAGE_RESOURCE_OPEN;
   rc=[self prepareActualResources:images];if(rc){stage=_resourceStage;goto stopped;}
   stage=NGP_BOOT_STAGE_SESSION;
   NGPBT(WORKER_STAGE,stage,0,0,0);
   if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_session_create_v1(&_session);[self endLane];if(rc)goto stopped;
   [self publishStage:stage status:rc terminal:NO];
   stage=NGP_BOOT_STAGE_PREPARE;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_prepare_v1(_session,_profile.bytes,images);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_GL;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=rt_gl_bind_native_v1(_capsule);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_ATTACH;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_attach_v1(_session,(__bridge void *)_binding);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_POWER_PREPARE;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_power_worker_prepare_v1(_lease,_incarnation,1,&_powerWorker);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_POWER_BIND;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_power_worker_bind_checkpoint_v1(_powerWorker);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_ACTIVATION;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=[_powerOwner requestActivationForPreparedWorker:_powerWorker];[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_WAIT;NGPBT(WORKER_STAGE,stage,0,0,0);rc=[self awaitActualPower];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_AUDIO_WAIT;NGPBT(WORKER_STAGE,stage,0,0,0);rc=[self awaitActualAudio];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_AUDIO_BIND;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_bind_miles_v1(_session,&_audioTicket);[self endLane];if(rc)goto stopped;
   stage=NGP_BOOT_STAGE_COMMON;NGPBT(WORKER_STAGE,stage,0,0,0);if(![self beginLane]){rc=NGP_BOOT_CONTROLLER_CANCELLED;goto stopped;}
   rc=ngp_app_engine_common_v1(_session,_powerWorker,_command.bytes,_commandCount,_commonBudget);
   [self endLane];if(rc)goto stopped;[self publishStage:stage status:rc terminal:NO];
   NGPBT(WORKER_STAGE,NGP_BOOT_STAGE_FRAME,0,0,0);
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
   (void)ngp_boot_failure_record_v1(&_firstFailure,stage,rc);
   NGPBT(WORKER_STOPPED,stage,rc,0,0);
   atomic_fetch_or_explicit(&_lane,NGP_LANE_CANCEL,memory_order_acq_rel);
   ngp_power_request_cancel_v1(_lease);
   NgpResourceSetV1 *resources=atomic_load_explicit(&_resources,memory_order_acquire);
   if(resources)(void)ngp_resources_cancel_v1(resources);
   [self publishStage:stage status:rc terminal:YES];
  }@catch(NSException *e){
   ngpbt_exception(NGP_BT_WORKER_EXCEPTION,e);
   (void)e;
   (void)ngp_boot_failure_record_v1(&_firstFailure,NGP_BOOT_STAGE_EXCEPTION,NGP_BOOT_CONTROLLER_EXCEPTION);
   /* Escaped provider exception is uncertain completion. Preserve BUSY if set:
    * no main pause, close, retry or freeing follows from this handler. */
   atomic_fetch_or_explicit(&_lane,NGP_LANE_CANCEL,memory_order_acq_rel);
   ngp_power_request_cancel_v1(_lease);
   NgpResourceSetV1 *resources=atomic_load_explicit(&_resources,memory_order_acquire);
   if(resources)(void)ngp_resources_cancel_v1(resources);
   @try {[self publishStage:NGP_BOOT_STAGE_EXCEPTION status:NGP_BOOT_CONTROLLER_EXCEPTION terminal:YES];}
   @catch(NSException *again){(void)again;}
  }
  NGPBT(PARK,0,0,0,0);
  [self parkActualThread];
 }
}
- (void)parkActualThread {
 /* Keep the ACTUAL engine pthread alive; retained NSThread alone is not enough.
  * There is no join/destruction/G2 or automatic resume in this first profile. */
 for(;;){struct timespec pause={3600,0};(void)nanosleep(&pause,NULL);}
}
- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
 if(_startAttempted)[self cancelBeforeLifecycleChange];
 [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}
@end
