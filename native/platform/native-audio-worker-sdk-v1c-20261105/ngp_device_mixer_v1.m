/* Source candidate only. Actual device route/callback behavior remains untested. */
#import "ngp_device_mixer_v1.h"
#import <AVFAudio/AVFAudio.h>
#import <AudioToolbox/AudioToolbox.h>
#import <TargetConditionals.h>
#import <dispatch/dispatch.h>
#include "ngp_device_mixer_render_v1.h"
#include <math.h>
#include <stdlib.h>
#if !TARGET_OS_IOS || TARGET_OS_SIMULATOR || TARGET_OS_MACCATALYST
#error Device target required; Simulator/Mac cannot certify this broker
#endif

typedef struct {
 NgpDeviceMixerRenderV1 render;
 _Atomic uint32_t phase,stop_requested,first_status;
 _Atomic(NgpVoiceServiceV1*) service;
 _Atomic uint64_t *intent_state;
 uint64_t epoch;
 NgpDeviceMixerFactsV1 facts;
} NgpDeviceMixerCapsuleV1;
/* C capsule and atomic intent storage are retained by the canonical broker for
 * process life. Revocation is not a graph mutation or a callback-drain proof. */
static void ngp_device_mixer_revoke_v1(NgpDeviceMixerCapsuleV1*c,uint32_t status,uint32_t reason){
 if(!c)return;
 atomic_store_explicit(&c->stop_requested,1,memory_order_seq_cst);
 uint32_t expected=0;atomic_compare_exchange_strong_explicit(&c->first_status,&expected,status,memory_order_seq_cst,memory_order_seq_cst);
 expected=0;atomic_compare_exchange_strong_explicit(&c->render.failed,&expected,NGP_DEVICE_MIXER_FAILED_V1,memory_order_seq_cst,memory_order_seq_cst);
 atomic_store_explicit(&c->phase,NGP_DEVICE_BROKER_FAILED_RETAINED_V1,memory_order_seq_cst);
 NgpVoiceServiceV1*s=atomic_load_explicit(&c->service,memory_order_seq_cst);
 if(s)(void)ngp_voice_service_v1_invalidate(s,reason);
}
static int current(NgpDeviceMixerCapsuleV1*c){
 return c && !atomic_load_explicit(&c->stop_requested,memory_order_seq_cst) &&
  (c->epoch&3u)==3u && atomic_load_explicit(c->intent_state,memory_order_seq_cst)==c->epoch;
}
static uint64_t publish_intent(_Atomic uint64_t*state,uint32_t flags){
 uint64_t old=atomic_load_explicit(state,memory_order_seq_cst);
 for(;;){uint64_t generation=old>>2;
  if(generation==UINT64_MAX>>2){
   uint64_t terminal=old&~UINT64_C(3);if(old==terminal)return 0;
   if(atomic_compare_exchange_weak_explicit(state,&old,terminal,memory_order_seq_cst,memory_order_seq_cst))return 0;
   continue;
  }
  uint64_t next=((generation+1)<<2)|(flags&3u);
  if(atomic_compare_exchange_weak_explicit(state,&old,next,memory_order_seq_cst,memory_order_seq_cst))return next;
 }
}
@interface NGPDeviceMixerIntentV1(){
@public _Atomic uint64_t _intentState;
 _Atomic(NgpDeviceMixerCapsuleV1*) _capsule;
}
@property(nonatomic,weak) UIWindowScene*scene;
@property(nonatomic,strong) NSMutableArray*observers;
@end
@implementation NGPDeviceMixerIntentV1
- (instancetype)initWithScene:(UIWindowScene*)scene {
 if(!NSThread.isMainThread||!scene)return nil;
 self=[super init];if(!self)return nil;
 atomic_init(&_intentState,0);atomic_init(&_capsule,NULL);self.scene=scene;self.observers=[NSMutableArray new];
 if(!self.observers)return nil;
 (void)publish_intent(&_intentState,scene.activationState==UISceneActivationStateForegroundActive?1:0);
 __weak NGPDeviceMixerIntentV1*weak=self;
 NSArray*names=@[UISceneDidActivateNotification,UISceneWillDeactivateNotification,UISceneDidEnterBackgroundNotification,UISceneDidDisconnectNotification];
 for(NSUInteger i=0;i<names.count;i++){
  BOOL activating=i==0;
  id token=[NSNotificationCenter.defaultCenter addObserverForName:names[i] object:scene queue:nil usingBlock:^(NSNotification*n){
   (void)n;NGPDeviceMixerIntentV1*owner=weak;if(!owner)return;
   uint32_t flags=NSThread.isMainThread&&activating&&owner.scene.activationState==UISceneActivationStateForegroundActive?1u:0u;
   (void)publish_intent(&owner->_intentState,flags);
   ngp_device_mixer_revoke_v1(atomic_load_explicit(&owner->_capsule,memory_order_seq_cst),NGP_DEVICE_BROKER_INTENT_V1,NGP_VOICE_LIFECYCLE_V1);
  }];
  if(!token){[self clearPlaybackIntent];return nil;}[self.observers addObject:token];
 }
 return self;
}
- (uint64_t)requestPlaybackIntent {
 if(!NSThread.isMainThread||!self.scene||self.scene.activationState!=UISceneActivationStateForegroundActive)return 0;
 NgpDeviceMixerCapsuleV1*c=atomic_load_explicit(&_capsule,memory_order_seq_cst);
 if(c)return current(c)?c->epoch:0; /* Repeated same user action is idempotent. */
 return publish_intent(&_intentState,3);
}
- (void)clearPlaybackIntent {
 uint32_t flags=NSThread.isMainThread&&self.scene&&self.scene.activationState==UISceneActivationStateForegroundActive?1u:0u;
 (void)publish_intent(&_intentState,flags);
 ngp_device_mixer_revoke_v1(atomic_load_explicit(&_capsule,memory_order_seq_cst),NGP_DEVICE_BROKER_INTENT_V1,NGP_VOICE_LIFECYCLE_V1);
}
- (void)dealloc {
 for(id token in self.observers)[NSNotificationCenter.defaultCenter removeObserver:token];
 (void)publish_intent(&_intentState,0);
 ngp_device_mixer_revoke_v1(atomic_load_explicit(&_capsule,memory_order_seq_cst),NGP_DEVICE_BROKER_INTENT_V1,NGP_VOICE_LIFECYCLE_V1);
}
@end

static char controlKey;
@interface NGPDeviceMixerCoordinatorV1:NSObject
@property(nonatomic,strong) dispatch_queue_t control;
@property(nonatomic,strong) AVAudioSession*session;
@property(nonatomic,assign) NgpDeviceMixerCapsuleV1*owner;
+ (instancetype)shared;
@end
@implementation NGPDeviceMixerCoordinatorV1
+ (instancetype)shared {
 static NGPDeviceMixerCoordinatorV1*owner;static dispatch_once_t once;
 dispatch_once(&once,^{@try{
  owner=[NGPDeviceMixerCoordinatorV1 new];owner.control=dispatch_queue_create("ngp.audio.mixer.canonical.v1",DISPATCH_QUEUE_SERIAL);
  if(owner.control)dispatch_queue_set_specific(owner.control,&controlKey,&controlKey,NULL);
  owner.session=AVAudioSession.sharedInstance;
 }@catch(NSException*e){(void)e;owner=nil;}});return owner;
}
@end
@interface NGPDeviceMixerBrokerV1(){@public NgpDeviceMixerCapsuleV1*_state;}
@property(nonatomic,strong) NGPDeviceMixerIntentV1*intent;
@property(nonatomic,strong) NGPDeviceMixerCoordinatorV1*coordinator;
@property(nonatomic,strong) AVAudioEngine*engine;
@property(nonatomic,strong) AVAudioSourceNode*source;
@property(nonatomic,strong) AVAudioFormat*format;
@property(nonatomic,strong) NSMutableArray*observers;
@property(nonatomic,strong) dispatch_source_t monitor;
@property(nonatomic,assign) BOOL activationAttempted,stopAttempted;
- (void)openOwned;
- (void)stopOwned;
- (void)observeOwned;
- (instancetype)initPrivate;
@end
static NGPDeviceMixerBrokerV1*canonicalBroker;
@implementation NGPDeviceMixerBrokerV1
+ (instancetype)openFromIntent:(NGPDeviceMixerIntentV1*)intent {
 @try {@autoreleasepool {
  if(!NSThread.isMainThread||!intent)return nil;
  uint64_t epoch=atomic_load_explicit(&intent->_intentState,memory_order_seq_cst);if((epoch&3u)!=3u)return nil;
  static _Atomic uint32_t attempted;uint32_t expected=0;
  if(!atomic_compare_exchange_strong_explicit(&attempted,&expected,1,memory_order_seq_cst,memory_order_seq_cst))return nil;
  /* Keep even failed partial construction; the one-shot attempt is consumed. */
  canonicalBroker=[NGPDeviceMixerBrokerV1 alloc];canonicalBroker=[canonicalBroker initPrivate];
  NGPDeviceMixerBrokerV1*canonical=canonicalBroker;
  if(!canonical)return nil;
  canonical.intent=intent;canonical.coordinator=NGPDeviceMixerCoordinatorV1.shared;
  NgpDeviceMixerCapsuleV1*c=calloc(1,sizeof(*c));canonical->_state=c;
  if(!c)return canonical;
  atomic_init(&c->phase,NGP_DEVICE_BROKER_OPENING_V1);atomic_init(&c->stop_requested,0);atomic_init(&c->first_status,0);
  atomic_init(&c->service,NULL);atomic_init(&c->render.busy,0);atomic_init(&c->render.failed,0);
  c->intent_state=&intent->_intentState;c->epoch=epoch;
  NgpDeviceMixerCapsuleV1*none=NULL;
  if(!atomic_compare_exchange_strong_explicit(&intent->_capsule,&none,c,memory_order_seq_cst,memory_order_seq_cst)||!current(c)||!canonical.coordinator.control||!canonical.coordinator.session){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_INTENT_V1,NGP_VOICE_LIFECYCLE_V1);return canonical;}
  NGPDeviceMixerBrokerV1*b=canonical;dispatch_async(b.coordinator.control,^{@autoreleasepool {[b openOwned];}});return canonical;
 }}@catch(NSException*e){
  (void)e;ngp_device_mixer_revoke_v1(canonicalBroker?canonicalBroker->_state:NULL,NGP_DEVICE_BROKER_EXCEPTION_V1,NGP_VOICE_CANCELLED_V1);
  @try {[canonicalBroker requestStop];}@catch(NSException*again){(void)again;}
  return nil;
 }
}
- (instancetype)initPrivate {return [super init];}
- (void)observeOwned {
 NGPDeviceMixerBrokerV1*b=self;self.observers=[NSMutableArray new];
 if(!self.observers)@throw [NSException exceptionWithName:@"NGPMixerAllocation" reason:nil userInfo:nil];
 NSArray*names=@[AVAudioSessionRouteChangeNotification,AVAudioSessionInterruptionNotification,AVAudioSessionMediaServicesWereLostNotification,AVAudioSessionMediaServicesWereResetNotification,AVAudioEngineConfigurationChangeNotification];
 for(NSUInteger i=0;i<names.count;i++){
  id object=i==4?self.engine:self.coordinator.session;
  id token=[NSNotificationCenter.defaultCenter addObserverForName:names[i] object:object queue:nil usingBlock:^(NSNotification*n){
   (void)n;ngp_device_mixer_revoke_v1(b->_state,NGP_DEVICE_BROKER_ROUTE_V1,NGP_VOICE_ROUTE_V1);
   @try {dispatch_async(b.coordinator.control,^{@autoreleasepool {[b stopOwned];}});}@catch(NSException*e){(void)e;}
  }];
  if(!token)@throw [NSException exceptionWithName:@"NGPMixerObserver" reason:nil userInfo:nil];[self.observers addObject:token];
 }
 self.monitor=dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,self.coordinator.control);
 if(!self.monitor)@throw [NSException exceptionWithName:@"NGPMixerTimer" reason:nil userInfo:nil];
 dispatch_source_set_timer(self.monitor,dispatch_time(DISPATCH_TIME_NOW,10*NSEC_PER_MSEC),10*NSEC_PER_MSEC,NSEC_PER_MSEC);
 dispatch_source_set_event_handler(self.monitor,^{@autoreleasepool {
  if(!current(b->_state)||atomic_load_explicit(&b->_state->render.failed,memory_order_seq_cst)){
   ngp_device_mixer_revoke_v1(b->_state,NGP_DEVICE_BROKER_GRAPH_V1,NGP_VOICE_CANCELLED_V1);[b stopOwned];
  }
 }});dispatch_resume(self.monitor);
}
- (void)openOwned {
 NgpDeviceMixerCapsuleV1*c=_state;
 @try {@autoreleasepool {
  if(dispatch_get_specific(&controlKey)!=&controlKey||!current(c)||self.coordinator.owner){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_CLOSED_V1,NGP_VOICE_CANCELLED_V1);return;}
  self.coordinator.owner=c;self.engine=[AVAudioEngine new];if(!self.engine)@throw [NSException exceptionWithName:@"NGPMixerEngine" reason:nil userInfo:nil];
  [self observeOwned];NSError*error=nil;AVAudioSession*s=self.coordinator.session;
  BOOL ok=[s setCategory:AVAudioSessionCategoryPlayback mode:AVAudioSessionModeDefault options:0 error:&error];
  if(ok&&!error)ok=[s setPreferredSampleRate:48000 error:&error];
  if(ok&&!error)ok=[s setPreferredIOBufferDuration:0.010 error:&error];
  if(ok&&!error&&current(c)){self.activationAttempted=YES;ok=[s setActive:YES withOptions:0 error:&error];}
  else ok=NO;
  if(!ok||error||!current(c)){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_SESSION_V1,NGP_VOICE_ROUTE_V1);[self stopOwned];return;}
  AVAudioFormat*hardware=[self.engine.outputNode inputFormatForBus:0];double rate=s.sampleRate;
  uint32_t channels=hardware.channelCount;
  if(!s.currentRoute.outputs.count||(rate!=44100&&rate!=48000)||hardware.sampleRate!=rate||
    (channels!=1&&channels!=2)||s.outputNumberOfChannels!=channels||hardware.commonFormat!=AVAudioPCMFormatFloat32||hardware.isInterleaved||!isfinite(s.IOBufferDuration)||s.IOBufferDuration<=0){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_ROUTE_V1,NGP_VOICE_ROUTE_V1);[self stopOwned];return;}
  NgpVoiceOutputV1 output={1,sizeof(output),(uint32_t)rate,channels};NgpVoiceReportV1 report={0};report.version=1;report.size=sizeof(report);NgpVoiceServiceV1*voice=NULL;
  int reserved=ngp_voice_service_v1_reserve(&output,&voice,&report);atomic_store_explicit(&c->service,voice,memory_order_seq_cst);
  if(reserved||!voice||!current(c)){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_GRAPH_V1,NGP_VOICE_CANCELLED_V1);[self stopOwned];return;}
  c->render.service=voice;c->render.channels=channels;
  self.format=[[AVAudioFormat alloc]initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:rate channels:channels interleaved:NO];
  if(!self.format)@throw [NSException exceptionWithName:@"NGPMixerFormat" reason:nil userInfo:nil];
  /* Captures only this retained C pointer. No ObjC messages/dispatch/allocations
   * or user callbacks appear inside the hardware render block. */
  self.source=[[AVAudioSourceNode alloc]initWithFormat:self.format renderBlock:^OSStatus(BOOL*isSilence,const AudioTimeStamp*stamp,AVAudioFrameCount frames,AudioBufferList*data){
   (void)stamp;if(!isSilence||!data){
    if(isSilence)*isSilence=YES;
    ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_GRAPH_V1,NGP_VOICE_CANCELLED_V1);return kAudio_ParamError;
   }
   if(!current(c))ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_INTENT_V1,NGP_VOICE_LIFECYCLE_V1);
   float*planes[2]={NULL,NULL};size_t sizes[2]={0,0};uint32_t counts[2]={0,0};
   if(data->mNumberBuffers==channels)for(uint32_t i=0;i<channels;i++){planes[i]=data->mBuffers[i].mData;sizes[i]=data->mBuffers[i].mDataByteSize;counts[i]=data->mBuffers[i].mNumberChannels;}
   uint32_t silent=1;int rc=ngp_device_mixer_render_v1(&c->render,planes,sizes,counts,data->mNumberBuffers,frames,&silent);*isSilence=silent?YES:NO;
   if(rc){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_GRAPH_V1,NGP_VOICE_CANCELLED_V1);return rc==NGP_DEVICE_MIXER_FRAMES_V1?kAudioUnitErr_TooManyFramesToProcess:kAudio_ParamError;}
   return noErr;
  }];
  if(!self.source)@throw [NSException exceptionWithName:@"NGPMixerSource" reason:nil userInfo:nil];
  AUAudioUnit*unit=self.source.AUAudioUnit;
  if(!unit||unit.renderResourcesAllocated)@throw [NSException exceptionWithName:@"NGPMixerResources" reason:nil userInfo:nil];
  unit.maximumFramesToRender=NGP_VOICE_MAX_RENDER_FRAMES_V1;
  if(!unit.maximumFramesToRender||unit.maximumFramesToRender>NGP_VOICE_MAX_RENDER_FRAMES_V1)@throw [NSException exceptionWithName:@"NGPMixerFrames" reason:nil userInfo:nil];
  [self.engine attachNode:self.source];[self.engine connect:self.source to:self.engine.outputNode format:self.format];[self.engine prepare];
  ok=[self.engine startAndReturnError:&error];AVAudioFormat*actual=[self.source outputFormatForBus:0];
  if(!ok||error||!self.engine.isRunning||!current(c)||actual.sampleRate!=rate||actual.channelCount!=channels||actual.commonFormat!=AVAudioPCMFormatFloat32||actual.isInterleaved||!unit.maximumFramesToRender||unit.maximumFramesToRender>NGP_VOICE_MAX_RENDER_FRAMES_V1){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_GRAPH_V1,NGP_VOICE_ROUTE_V1);[self stopOwned];return;}
  c->facts=(NgpDeviceMixerFactsV1){1,sizeof(c->facts),NGP_DEVICE_BROKER_ACTIVE_V1,(uint32_t)rate,channels,unit.maximumFramesToRender,c->epoch,voice};
  uint32_t expected=NGP_DEVICE_BROKER_OPENING_V1;
  if(!current(c)||!atomic_compare_exchange_strong_explicit(&c->phase,&expected,NGP_DEVICE_BROKER_ACTIVE_V1,memory_order_seq_cst,memory_order_seq_cst)){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_CLOSED_V1,NGP_VOICE_CANCELLED_V1);[self stopOwned];}
 }}@catch(NSException*e){(void)e;ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_EXCEPTION_V1,NGP_VOICE_CANCELLED_V1);[self stopOwned];}
}
- (void)stopOwned {
 NgpDeviceMixerCapsuleV1*c=_state;if(!c)return;
 if(dispatch_get_specific(&controlKey)!=&controlKey){ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_EXCEPTION_V1,NGP_VOICE_CANCELLED_V1);return;}
 if(self.stopAttempted)return;self.stopAttempted=YES;
 @try {@autoreleasepool {
  if(self.monitor){dispatch_source_cancel(self.monitor);self.monitor=nil;}
  [self.engine stop];BOOL stopped=!self.engine||!self.engine.isRunning,lease=NO;
  if(self.coordinator.owner==c){
   if(self.activationAttempted){NSError*error=nil;BOOL deactivated=[self.coordinator.session setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&error];lease=deactivated&&!error;}
   else lease=YES;
   if(stopped&&lease)self.coordinator.owner=NULL;
  }
  if(stopped&&lease)atomic_store_explicit(&c->phase,NGP_DEVICE_BROKER_STOPPED_RETAINED_V1,memory_order_seq_cst);
  /* Keep engine/node/observers/intent/C memory, even after observed stop. No
   * reset, detach, free, restart or whole-callback quiescence ACK is emitted. */
 }}@catch(NSException*e){(void)e;ngp_device_mixer_revoke_v1(c,NGP_DEVICE_BROKER_EXCEPTION_V1,NGP_VOICE_CANCELLED_V1);}
}
- (int)copyDeviceFacts:(NgpDeviceMixerFactsV1*)out {
 if(!out||out->version!=1||out->size!=sizeof(*out))return NGP_DEVICE_BROKER_ARGUMENT_V1;
 NgpDeviceMixerCapsuleV1*c=_state;if(!c)return NGP_DEVICE_BROKER_ALLOC_V1;
 uint32_t phase=atomic_load_explicit(&c->phase,memory_order_seq_cst);
 if(phase==NGP_DEVICE_BROKER_OPENING_V1)return NGP_DEVICE_BROKER_PENDING_V1;
 if(phase!=NGP_DEVICE_BROKER_ACTIVE_V1||!current(c)||atomic_load_explicit(&c->render.failed,memory_order_seq_cst))return NGP_DEVICE_BROKER_CLOSED_V1;
 NgpDeviceMixerFactsV1 facts=c->facts;
 if(atomic_load_explicit(&c->phase,memory_order_seq_cst)!=NGP_DEVICE_BROKER_ACTIVE_V1||!current(c)||atomic_load_explicit(&c->render.failed,memory_order_seq_cst))return NGP_DEVICE_BROKER_CLOSED_V1;
 *out=facts;return NGP_DEVICE_BROKER_OK_V1;
}
- (void)requestStop {
 ngp_device_mixer_revoke_v1(_state,NGP_DEVICE_BROKER_CLOSED_V1,NGP_VOICE_CANCELLED_V1);
 @try {NGPDeviceMixerBrokerV1*b=self;if(self.coordinator.control)dispatch_async(self.coordinator.control,^{@autoreleasepool {[b stopOwned];}});}@catch(NSException*e){(void)e;}
}
@end
