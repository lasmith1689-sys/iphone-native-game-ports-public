/* Own real UIKit entry/scene construction. Local candidate; no game claim. */
#import "NGPRetainedGameControllerV2.h"
#include "ngp_import_selector_v1.h"
/* Build 9 adapter enters the verified container directory on success. */
extern int32_t ngp_import_select_w1(int, char[37], NgpImportSelectReportV1 *);
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#import "ngp_boot_trace_objc_v1.h"
#include "ngp_start_admission_v1.h"
/* NGP-BOOT-TRACE-V4 BEGIN: trace-only app helpers; no host decision reads them. */
static NSTimer *ngpbt_heartbeat_timer;
static uint64_t ngpbt_heartbeat_ticks;
static void ngpbt_heartbeat_tick(UIWindow *window) {
 if(!NSThread.isMainThread)return;
 uint64_t tick=++ngpbt_heartbeat_ticks;
 if(tick>NGP_BOOT_TRACE_MAX_HEARTBEATS){[ngpbt_heartbeat_timer invalidate];ngpbt_heartbeat_timer=nil;return;}
 @try {
  UIWindowScene *scene=window.windowScene;UIViewController *root=window.rootViewController;
  UIView *view=root.viewIfLoaded;CGRect wb=window.bounds;
  int64_t flags=(window?1:0)|(window.hidden?2:0)|(window.isKeyWindow?4:0)|(root?8:0)|(view?16:0)|
   (view&&view.window==window?32:0)|(view.hidden?64:0);
  ngp_boot_trace_heartbeat_v1(NGP_BT_HB_WINDOW,1,flags,ngpbt_pt(wb.size.width),ngpbt_pt(wb.size.height),(int64_t)tick);
  ngp_boot_trace_heartbeat_v1(NGP_BT_HB_STATE,0,(int64_t)UIApplication.sharedApplication.applicationState,
   scene?(int64_t)scene.activationState:-1,scene?(int64_t)scene.effectiveGeometry.interfaceOrientation:-1,(int64_t)tick);
  UILabel *label=nil;
  for(UIView *candidate in view.subviews.reverseObjectEnumerator)if([candidate isKindOfClass:UILabel.class]){label=(UILabel *)candidate;break;}
  if(!label){ngp_boot_trace_heartbeat_v1(NGP_BT_HB_FLAGS,0,0,-1,-1,(int64_t)tick);return;}
  CGRect bounds=label.bounds;CGRect frame=[label convertRect:bounds toView:nil];
  CGSize fit=[label sizeThatFits:CGSizeMake(bounds.size.width,CGFLOAT_MAX)];
  CGFloat tw=fmin(fit.width,bounds.size.width),th=fmin(fit.height,bounds.size.height);
  /* ESTIMATE only: assumes natural/left alignment and vertical centering. */
  CGRect text=[label convertRect:CGRectMake(bounds.origin.x,bounds.origin.y+(bounds.size.height-th)/2,tw,th) toView:nil];
  ngp_boot_trace_heartbeat_v1(NGP_BT_HB_LABEL_FRAME,0,ngpbt_pt(frame.origin.x),ngpbt_pt(frame.origin.y),ngpbt_pt(frame.size.width),ngpbt_pt(frame.size.height));
  ngp_boot_trace_heartbeat_v1(NGP_BT_HB_TEXT_EST,0,ngpbt_pt(text.origin.x),ngpbt_pt(text.origin.y),ngpbt_pt(text.size.width),ngpbt_pt(text.size.height));
  int64_t lf=1|(label.hidden?2:0)|(label.alpha<=0.01?4:0)|(label.window==window?8:0)|(view.subviews.lastObject==label?16:0)|
   (label.text.length?32:0)|(CGRectIntersectsRect(text,wb)?64:0)|(CGRectContainsRect(wb,text)?128:0);
  ngp_boot_trace_heartbeat_v1(NGP_BT_HB_FLAGS,0,lf,(int64_t)llround(label.alpha*1000),(int64_t)llround(view.alpha*1000),(int64_t)llround(window.alpha*1000));
 }@catch(NSException *e){(void)e;ngp_boot_trace_heartbeat_v1(NGP_BT_HB_FLAGS,0,-1,-1,-1,(int64_t)tick);}
}
static void ngpbt_start_heartbeat(UIWindow *window) {
 @try {
  if(!NSThread.isMainThread||ngpbt_heartbeat_timer||!window)return;
  __weak UIWindow *weakWindow=window;
  NSTimer *timer=[NSTimer timerWithTimeInterval:1.0 repeats:YES block:^(NSTimer *t){(void)t;ngpbt_heartbeat_tick(weakWindow);}];
  timer.tolerance=0.1;[NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
  ngpbt_heartbeat_timer=timer;
  NGPBT(HB_SCHEDULED,1,0,0,0);
 }@catch(NSException *e){(void)e;NGPBT(HB_SCHEDULED,0,0,0,0);}
}
static void ngpbt_open_and_mark_main(void) {
 @try {
  NSURL *caches=[NSFileManager.defaultManager URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask].firstObject;
  NSDictionary *info=NSBundle.mainBundle.infoDictionary;
  id bundle=info[@"CFBundleVersion"],version=info[@"CFBundleShortVersionString"];
  const char *path=caches.isFileURL?caches.fileSystemRepresentation:NULL;
  int rc=ngp_boot_trace_open_posix_v1(path,[bundle isKindOfClass:NSString.class]?[(NSString *)bundle UTF8String]:NULL,
   [version isKindOfClass:NSString.class]?[(NSString *)version UTF8String]:NULL,NGP_BOOT_TRACE_EXPECTED_BUILD);
  NGPBT(MAIN,rc,0,0,0);
 }@catch(NSException *e){(void)e;}
}
/* NGP-BOOT-TRACE-V4 END */

@interface NGPNativeSceneDelegateV3 : UIResponder<UIWindowSceneDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) NGPRetainedGameControllerV2 *controller;
@property(nonatomic) BOOL startRequested;
@property(nonatomic,strong) NSTimer *startAdmissionTimer;
@property(nonatomic) NgpImportSelectReportV1 importSelectionReport;
- (void)cancelStartupAndEngine;
@end
static NGPNativeSceneDelegateV3 *retained_scene;
static BOOL scene_claimed; /* main only, no scene replacement or restart */
static void cancel_before_app_lifecycle(void) {
 if(NSThread.isMainThread)[retained_scene cancelStartupAndEngine];
}
@implementation NGPNativeSceneDelegateV3 {
 NGPStartAdmissionV1 _startAdmission;
}
- (void)cancelStartupAndEngine {
 [_startAdmissionTimer invalidate];
 _startAdmissionTimer=nil;
 _startAdmission.finished=1;
 [self.controller cancelBeforeLifecycleChange];
}
- (void)pollStartAdmission {
 UIWindow *window=self.window;
 UIWindowScene *scene=window.windowScene;
 NGPRetainedGameControllerV2 *controller=self.controller;
 UIView *view=controller.viewIfLoaded;
 CGRect bounds=view.bounds;
 BOOL ready=scene && controller && window.rootViewController==controller && !window.hidden &&
   view.window==window && CGSizeEqualToSize(bounds.size,window.bounds.size) &&
   scene.activationState==UISceneActivationStateForegroundActive &&
   UIApplication.sharedApplication.applicationState==UIApplicationStateActive &&
   scene.effectiveGeometry.interfaceOrientation==UIInterfaceOrientationLandscapeRight;
 int decision=ngp_start_admission_v1_poll(&_startAdmission,ready,bounds.size.width,bounds.size.height);
 if(decision==NGP_START_WAIT)return;
 [self.startAdmissionTimer invalidate];
 self.startAdmissionTimer=nil;
 if(decision==NGP_START_READY){self.startRequested=YES;[controller startInActiveScene];}
 else if(decision==NGP_START_EXPIRED)[self cancelStartupAndEngine];
}
- (void)showSetupFailure:(NSString *)message {
 NGPBT(SETUP_UI,0,0,0,0);
 UIViewController *fallback=[[UIViewController alloc] init];
 fallback.view.backgroundColor=UIColor.blackColor;
 UILabel *label=[[UILabel alloc] initWithFrame:CGRectInset(self.window.bounds,24,24)];
 label.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
 label.textColor=UIColor.whiteColor;label.numberOfLines=0;label.text=message;
 [fallback.view addSubview:label];self.window.rootViewController=fallback;
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
 (void)session;(void)options;
 NGPBT(WILL_CONNECT,NSThread.isMainThread,scene_claimed,[scene isKindOfClass:UIWindowScene.class],0);
 if(!NSThread.isMainThread||scene_claimed||![scene isKindOfClass:UIWindowScene.class])return;
 scene_claimed=YES;retained_scene=self;
 self.window=[[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
 ngpbt_window(NGP_BT_WINDOW,self.window);
 ngpbt_start_heartbeat(self.window);
 @try {
  NSFileManager *files=NSFileManager.defaultManager;
  NSURL *support=[files URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
  NSURL *base=[support URLByAppendingPathComponent:@"NativeCoD" isDirectory:YES];
  /* The offline importer commits only active-import.json outside strict roots.
   * No root is created here. Completed version/ancestors are trusted immutable;
   * no AFC/Files/iCloud writer may mutate a selected version while app runs. */
  const char *rootPath=base.fileSystemRepresentation;
  NgpImportSelectReportV1 selectionReport={NGP_IMPORT_SELECT_ARGUMENT,0,NGP_IMPORT_SELECT_STAGE_ARGUMENT,0};
  int32_t selectionStatus=NGP_IMPORT_SELECT_ARGUMENT;int rootFD=-1;char transaction[37]={0};
  if(support&&base&&rootPath){
   rootFD=open(rootPath,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
   if(rootFD<0){selectionStatus=NGP_IMPORT_SELECT_IO;selectionReport.native_error=errno;selectionReport.stage=NGP_IMPORT_SELECT_STAGE_ROOT;}
   else{
    selectionStatus=ngp_import_select_w1(rootFD,transaction,&selectionReport);
    if(close(rootFD)&&!selectionStatus){selectionStatus=NGP_IMPORT_SELECT_IO;selectionReport.native_error=errno;selectionReport.stage=NGP_IMPORT_SELECT_STAGE_CLOSE;}
    /* An uncertain close is not retried. No controller is made from failed I/O. */
   }
   NGPBT(ROOT_OPEN,rootFD>=0,rootFD<0?selectionReport.native_error:0,0,0);
  }
  selectionReport.status=selectionStatus;self.importSelectionReport=selectionReport;
  NGPBT(SELECT,selectionStatus,selectionReport.stage,selectionReport.native_error,selectionReport.bytes_read);
  if(selectionStatus){
   [self showSetupFailure:[NSString stringWithFormat:@"No completed private import selected (stage %u, status %d, native %d). Close the app and complete the offline import first.",selectionReport.stage,selectionReport.status,selectionReport.native_error]];
  }else{
   NSString *uuid=[[NSString alloc] initWithBytes:transaction length:36 encoding:NSASCIIStringEncoding];
   NSURL *version=uuid?[[base URLByAppendingPathComponent:@"Imports" isDirectory:YES] URLByAppendingPathComponent:uuid isDirectory:YES]:nil;
   NSURL *images=[version URLByAppendingPathComponent:@"images" isDirectory:YES];
   NSURL *assets=[version URLByAppendingPathComponent:@"assets" isDirectory:YES];
   if(!uuid||!images||!assets){
    [self showSetupFailure:@"The selected private import path could not be constructed. Close and relaunch after setup."];
   }else{
    static const char profile[]="cod2003-retained-v2";
    NSData *profileBytes=[NSData dataWithBytes:profile length:sizeof(profile)];
    /* A selector is not an asset credential. The unchanged controller performs
     * actual resource validate/cwd/borrow before prepare. No path override. */
    NGPBT(CTL_INIT_ENTER,0,0,0,0);
    self.controller=[[NGPRetainedGameControllerV2 alloc] initWithWindow:self.window
      imageDirectoryURL:images assetDirectoryURL:assets profileBytes:profileBytes
      commandBytes:[NSData data] commonBudget:INT64_C(1000000000) frameBudget:INT64_C(100000000)];
    NGPBT(CTL_INIT_EXIT,self.controller!=nil,0,0,0);
    NGPBT(ROOTVC_ENTER,self.controller!=nil,0,0,0);
    if(self.controller)self.window.rootViewController=self.controller;
    else [self showSetupFailure:@"Native game setup could not be constructed. Close and relaunch after setup."];
    NGPBT(ROOTVC_EXIT,self.controller!=nil,0,0,0);
   }
  }
  NGPBT(KEYVIS_ENTER,0,0,0,0);
  [self.window makeKeyAndVisible];
  ngpbt_window(NGP_BT_KEYVIS_EXIT,self.window);
 }@catch(NSException *exception){
  ngpbt_exception(NGP_BT_SCENE_EXCEPTION,exception);
  (void)exception;[self.controller cancelBeforeLifecycleChange];
  /* Any partial controller/window remains retained. No substitute engine. */
 }
}
- (void)sceneDidBecomeActive:(UIScene *)scene {
 NGPBT(ACTIVE_ENTER,scene==self.window.windowScene,self.startRequested,0,0);
 if(scene==self.window.windowScene && self.controller && !self.startAdmissionTimer && !_startAdmission.finished){
  __weak NGPNativeSceneDelegateV3 *owner=self;
  self.startAdmissionTimer=[NSTimer timerWithTimeInterval:0.05 repeats:YES block:^(NSTimer *timer){
   (void)timer;
   [owner pollStartAdmission];
  }];
  [NSRunLoop.mainRunLoop addTimer:self.startAdmissionTimer forMode:NSRunLoopCommonModes];
 }
 NGPBT(ACTIVE_EXIT,self.startRequested,0,0,0);
}
- (void)sceneWillResignActive:(UIScene *)scene {
 NGPBT(RESIGN,0,0,0,0);
 (void)scene;[self cancelStartupAndEngine];
}
- (void)sceneDidEnterBackground:(UIScene *)scene {
 NGPBT(BACKGROUND,0,0,0,0);
 (void)scene;[self cancelStartupAndEngine];
}
- (void)sceneDidDisconnect:(UIScene *)scene {
 NGPBT(DISCONNECT,0,0,0,0);
 (void)scene;[self cancelStartupAndEngine];
 /* Never clear window/controller or infer G2 from UIKit disconnection. */
}
- (void)windowScene:(UIWindowScene *)scene didUpdateCoordinateSpace:(id<UICoordinateSpace>)previousCoordinateSpace
 interfaceOrientation:(UIInterfaceOrientation)previousInterfaceOrientation traitCollection:(UITraitCollection *)previousTraitCollection {
 ngpbt_coord(scene,previousInterfaceOrientation,self.startRequested);
 (void)previousCoordinateSpace;
 /* Real geometry changes and all lifecycle interruptions remain terminal. */
 if(self.startRequested && ![self.controller hasUnchangedDrawableInScene:scene
    previousOrientation:previousInterfaceOrientation previousScale:previousTraitCollection.displayScale])
  [self.controller cancelBeforeLifecycleChange];
}
@end

@interface NGPNativeAppDelegateV3 : UIResponder<UIApplicationDelegate>
@end
@implementation NGPNativeAppDelegateV3
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options {
 NGPBT(DID_FINISH,0,0,0,0);
 (void)application;(void)options;return YES;
}
- (UISceneConfiguration *)application:(UIApplication *)application
 configurationForConnectingSceneSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
 NGPBT(SCENE_CONFIG,0,0,0,0);
 (void)application;(void)options;
 UISceneConfiguration *configuration=[[UISceneConfiguration alloc] initWithName:@"Native engine" sessionRole:session.role];
 configuration.delegateClass=NGPNativeSceneDelegateV3.class;return configuration;
}
- (void)applicationWillResignActive:(UIApplication *)application {(void)application;NGPBT(APP_RESIGN,0,0,0,0);cancel_before_app_lifecycle();}
- (void)applicationDidEnterBackground:(UIApplication *)application {(void)application;NGPBT(APP_BACKGROUND,0,0,0,0);cancel_before_app_lifecycle();}
- (void)applicationWillTerminate:(UIApplication *)application {(void)application;NGPBT(APP_TERMINATE,0,0,0,0);cancel_before_app_lifecycle();}
@end

int main(int argc,char *argv[]) {
 @autoreleasepool {ngpbt_open_and_mark_main();return UIApplicationMain(argc,argv,nil,NSStringFromClass(NGPNativeAppDelegateV3.class));}
}
