/* Owned UIKit/Metal diagnostic only. No translated game or recovered source. */
// MD_SIM_AUTOSTART_BEGIN_header
#import <TargetConditionals.h>
// MD_SIM_AUTOSTART_END_header
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <dispatch/dispatch.h>
#include <math.h>
#include <string.h>
#include "shell_policy.h"

@interface MDMetalView : UIView
@end
@implementation MDMetalView
+ (Class)layerClass { return CAMetalLayer.class; }
@end

@interface MDController : UIViewController {
    MDPolicy _policy;
// MD_SIM_AUTOSTART_BEGIN_state
#if TARGET_OS_SIMULATOR
    BOOL _simAutostartScheduled, _simAutostartFinished, _simHadReadyGeometry;
    unsigned _simReadinessPolls;
    CFTimeInterval _simReadinessDeadline;
    MDGeometry _simPreviousGeometry;
#endif
// MD_SIM_AUTOSTART_END_state
    id<MTLDevice> _device;
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLTexture> _offscreen;
    id<MTLBuffer> _readback;
    id<CAMetalDrawable> _drawable;
    id<MTLCommandBuffer> _command;
    MDMetalView *_metalView;
    UILabel *_status;
    UIButton *_start;
    NSMutableArray<NSDictionary *> *_events;
    NSString *_lastDetail;
}
- (void)cancelForReason:(NSString *)reason;
- (void)refreshAvailability;
- (CAMetalLayer *)metalLayer;
- (MDGeometry)captureGeometry;
- (void)publish:(NSString *)event detail:(NSString *)detail;
- (void)refuse:(NSString *)detail;
- (BOOL)encodeTarget:(id<MTLTexture>)texture;
- (void)startDiagnostic;
- (void)applicationResigned:(NSNotification *)note;
// MD_SIM_AUTOSTART_BEGIN_declarations
#if TARGET_OS_SIMULATOR
- (void)scheduleSimulatorAutostartIfRequested;
- (void)pollSimulatorAutostartReadiness;
#endif
// MD_SIM_AUTOSTART_END_declarations
@end

@implementation MDController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.blackColor;
    _events = [NSMutableArray array];
    _metalView = [[MDMetalView alloc] initWithFrame:self.view.bounds];
    _metalView.userInteractionEnabled = NO;
    [self.view addSubview:_metalView];
    _status = [[UILabel alloc] init];
    _status.numberOfLines = 0;
    _status.font = [UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular];
    _status.textColor = UIColor.whiteColor;
    _status.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8];
    _status.accessibilityIdentifier = @"diagnostic-status";
    [self.view addSubview:_status];
    _start = [UIButton buttonWithType:UIButtonTypeSystem];
    [_start setTitle:@"Run one synthetic Metal frame" forState:UIControlStateNormal];
    _start.backgroundColor = UIColor.whiteColor;
    _start.accessibilityIdentifier = @"diagnostic-start";
    [_start addTarget:self action:@selector(startDiagnostic) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_start];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationResigned:)
        name:UIApplicationWillResignActiveNotification object:nil];
    [self publish:@"ready" detail:@"Owned infrastructure only; no game is loaded."];
// MD_SIM_AUTOSTART_BEGIN_hook
#if TARGET_OS_SIMULATOR
    [self scheduleSimulatorAutostartIfRequested];
#endif
// MD_SIM_AUTOSTART_END_hook
}

// MD_SIM_AUTOSTART_BEGIN_implementation
#if TARGET_OS_SIMULATOR
- (void)scheduleSimulatorAutostartIfRequested {
    NSAssert(NSThread.isMainThread, @"simulator fixture uses existing main-thread authority");
    if (_simAutostartScheduled ||
        ![NSProcessInfo.processInfo.arguments containsObject:@"--owned-metal-simulator-autostart-v1"])
        return;
    _simAutostartScheduled = YES;
    _simReadinessDeadline = CACurrentMediaTime() + 15.0;
    [self publish:@"simulator-autostart-waiting" detail:@"Explicit simulator fixture: waiting for two matching foreground geometry captures."];
    __weak MDController *weakOwner = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 10), dispatch_get_main_queue(), ^{
        MDController *owner = weakOwner;
        if (owner) [owner pollSimulatorAutostartReadiness];
    });
}

- (void)pollSimulatorAutostartReadiness {
    NSAssert(NSThread.isMainThread, @"simulator fixture uses existing main-thread authority");
    if (!_simAutostartScheduled || _simAutostartFinished) return;
    if (_policy.begun || _policy.failed || _policy.cancelled) {
        _simAutostartFinished = YES;
        [self publish:@"simulator-autostart-superseded" detail:@"Existing lifetime already consumed; fixture does not start or resume it."];
        return;
    }
    ++_simReadinessPolls;
    CFTimeInterval now = CACurrentMediaTime();
    if (_simReadinessPolls >= 150 || !isfinite(now) || !isfinite(_simReadinessDeadline) || now >= _simReadinessDeadline) {
        _simAutostartFinished = YES;
        [self publish:@"simulator-autostart-timeout" detail:@"Foreground/geometry readiness not established within the bounded fixture wait."];
        [self refuse:@"Simulator autostart readiness expired; diagnostic was not invoked."];
        return;
    }
    [self.view layoutIfNeeded];
    MDGeometry current = [self captureGeometry];
    now = CACurrentMediaTime();
    if (!isfinite(now) || now >= _simReadinessDeadline) {
        _simAutostartFinished = YES;
        [self publish:@"simulator-autostart-timeout" detail:@"Readiness deadline expired during layout; diagnostic was not invoked."];
        [self refuse:@"Simulator autostart readiness expired; diagnostic was not invoked."];
        return;
    }
    if (md_geometry_valid(&current)) {
        if (_simHadReadyGeometry && md_geometry_equal(&_simPreviousGeometry, &current)) {
            _simAutostartFinished = YES;
            [self publish:@"simulator-autostart-ready" detail:@"Two matching foreground geometry captures; calling the unchanged diagnostic entry."];
            [self startDiagnostic]; /* Rechecks admission again inside the exact original entry. */
            return;
        }
        _simPreviousGeometry = current;
        _simHadReadyGeometry = YES;
    } else {
        _simHadReadyGeometry = NO;
    }
    __weak MDController *weakOwner = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 10), dispatch_get_main_queue(), ^{
        MDController *owner = weakOwner;
        if (owner) [owner pollSimulatorAutostartReadiness];
    });
}
#endif

// MD_SIM_AUTOSTART_END_implementation
- (void)applicationResigned:(NSNotification *)note {
    (void)note;
    [self cancelForReason:@"application resigned active"];
}

- (CAMetalLayer *)metalLayer { return (CAMetalLayer *)_metalView.layer; }

- (MDGeometry)captureGeometry {
    NSAssert(NSThread.isMainThread, @"main-thread geometry authority");
    MDGeometry g = {0};
    UIWindow *window = self.view.window;
    UIWindowScene *scene = window.windowScene;
    CAMetalLayer *layer = self.metalLayer;
    CGRect bounds = _metalView.bounds;
    CGSize pixels = layer.drawableSize;
    double w = pixels.width, h = pixels.height;
    if (!window || !scene || !isfinite(w) || !isfinite(h) ||
        w < 1 || h < 1 || w > 8192 || h > 8192 || floor(w) != w || floor(h) != h)
        return g;
    g.scene = (uintptr_t)(__bridge void *)scene;
    g.window = (uintptr_t)(__bridge void *)window;
    g.points_w = bounds.size.width;
    g.points_h = bounds.size.height;
    g.scale = layer.contentsScale;
    g.pixels_w = (uint32_t)w;
    g.pixels_h = (uint32_t)h;
    g.orientation = (int)scene.effectiveGeometry.interfaceOrientation;
    g.foreground = UIApplication.sharedApplication.applicationState == UIApplicationStateActive &&
        scene.activationState == UISceneActivationStateForegroundActive;
    g.attached = window.isKeyWindow && !window.hidden && window.alpha == 1 &&
        window.rootViewController == self && self.presentedViewController == nil &&
        _metalView.window == window && !_metalView.hidden && _metalView.alpha == 1 &&
        !self.view.hidden && self.view.alpha == 1 &&
        CGAffineTransformIsIdentity(self.view.transform) &&
        CGAffineTransformIsIdentity(_metalView.transform) &&
        CATransform3DIsIdentity(layer.transform) &&
        CGRectEqualToRect(self.view.bounds, window.bounds) &&
        CGRectEqualToRect(_metalView.frame, self.view.bounds) &&
        bounds.origin.x == 0 && bounds.origin.y == 0 &&
        layer.contentsScale == scene.traitCollection.displayScale;
    return g;
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    _metalView.frame = self.view.bounds;
    UIWindowScene *scene = self.view.window.windowScene;
    if (scene) {
        double scale = scene.traitCollection.displayScale;
        double w = _metalView.bounds.size.width * scale;
        double h = _metalView.bounds.size.height * scale;
        if (isfinite(w) && isfinite(h) && w >= 1 && h >= 1 && w <= 8192 && h <= 8192) {
            self.metalLayer.contentsScale = scale;
            self.metalLayer.drawableSize = CGSizeMake(round(w), round(h));
        }
    }
    UIEdgeInsets safe = self.view.safeAreaInsets;
    CGFloat width = MAX(0, self.view.bounds.size.width - safe.left - safe.right - 24);
    _status.frame = CGRectMake(safe.left + 12, safe.top + 8, width, 170);
    _start.frame = CGRectMake(safe.left + 12,
        MAX(safe.top + 180, self.view.bounds.size.height - safe.bottom - 64), width, 48);
    if (_policy.begun) {
        MDGeometry current = [self captureGeometry];
        if (!md_geometry_equal(&_policy.geometry, &current))
            [self cancelForReason:@"layout, scene, or geometry changed"];
    }
    [self refreshAvailability];
}

- (void)refreshAvailability {
    NSAssert(NSThread.isMainThread, @"main-thread admission authority");
    if (!self.isViewLoaded) return;
    MDGeometry g = [self captureGeometry];
    _start.enabled = !_policy.begun && !_policy.failed && md_geometry_valid(&g);
}

- (void)publish:(NSString *)event detail:(NSString *)detail {
    NSAssert(NSThread.isMainThread, @"main-thread report authority");
    _lastDetail = detail ?: @"";
    NSDictionary *row = @{
// MD_SIM_AUTOSTART_BEGIN_command_report
#if TARGET_OS_SIMULATOR
        @"simulatorPresentationObservationAvailable": @NO,
        @"simulatorCommandSamplesTerminalObserved": @(_policy.completion_observed),
        @"simulatorCommandSamplesSucceeded": @(_policy.completion_observed && _policy.gpu_completed &&
            _policy.pixels_matched && !_policy.failed && !_policy.cancelled),
#endif
// MD_SIM_AUTOSTART_END_command_report
        @"event": event, @"detail": _lastDetail,
        @"begun": @(_policy.begun), @"committed": @(_policy.committed),
        @"cancelled": @(_policy.cancelled), @"failed": @(_policy.failed),
        @"completionObserved": @(_policy.completion_observed),
        @"gpuCompleted": @(_policy.gpu_completed), @"samplePixelsMatched": @(_policy.pixels_matched),
        @"presentationObserved": @(_policy.presentation_observed),
        @"presentationValid": @(_policy.presentation_valid),
        @"presentedTime": isfinite(_policy.presented_time) ? @(_policy.presented_time) : NSNull.null,
        @"combinedSuccess": @(md_success(&_policy))
    };
    if (_events.count < 32) [_events addObject:row];
    NSDictionary *report = @{
        @"schema": @"owned-metal-diagnostic-v1", @"engineLoaded": @NO,
        @"readbackSource": @"independent 64x64 offscreen render target, not the drawable",
        @"readbackSamples": @"BGRA blue corner and green center",
        @"lifetime": @"single attempt; no resume; resources retained until process exit",
        @"drawablePixels": @[@(_policy.geometry.pixels_w), @(_policy.geometry.pixels_h)],
        @"events": [_events copy]
    };
    NSError *error = nil;
    NSData *json = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:&error];
    NSURL *documents = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    if (json && documents) {
        [json writeToURL:[documents URLByAppendingPathComponent:@"metal-diagnostic.json"] options:NSDataWritingAtomic error:&error];
    }
    NSLog(@"OwnedMetalDiagnostic %@ %@", row, error ?: @"report persisted");
    _status.text = [NSString stringWithFormat:
        @"Synthetic Metal diagnostic\nCommand committed: %d\nGPU completion: %d; samples: %d\nActual present callback: %d\nCombined success: %d; cancelled: %d\n%@",
        _policy.committed, _policy.gpu_completed, _policy.pixels_matched,
        _policy.presentation_valid, md_success(&_policy), _policy.cancelled, _lastDetail];
    [self refreshAvailability];
}

- (void)cancelForReason:(NSString *)reason {
    NSAssert(NSThread.isMainThread, @"main-thread cancellation authority");
    if (!_policy.begun || _policy.cancelled) return;
    md_cancel(&_policy);
    [self publish:@"cancelled" detail:reason];
    /* Never release, wait for, pretend to cancel, or resubmit in-flight GPU work. */
}

- (void)refuse:(NSString *)detail {
    md_fail(&_policy);
    [self publish:@"refused" detail:detail];
}

- (BOOL)encodeTarget:(id<MTLTexture>)texture {
    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 1, 1);
    id<MTLRenderCommandEncoder> encoder = [_command renderCommandEncoderWithDescriptor:pass];
    if (!encoder) return NO;
    [encoder setRenderPipelineState:_pipeline];
    [encoder setCullMode:MTLCullModeNone];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [encoder endEncoding];
    return YES;
}

- (void)startDiagnostic {
    NSAssert(NSThread.isMainThread, @"main-thread start authority");
    if (_policy.begun) return;
    [self.view layoutIfNeeded];
    MDGeometry startGeometry = [self captureGeometry];
    if (!md_begin(&_policy, &startGeometry)) { [self refuse:@"foreground or geometry admission failed"]; return; }
    [self publish:@"begun" detail:@"One attempt admitted; no automatic restart."];
    _device = MTLCreateSystemDefaultDevice();
    _queue = [_device newCommandQueue];
    if (!_device || !_queue) { [self refuse:@"Metal device or command queue unavailable"]; return; }
    CAMetalLayer *layer = self.metalLayer;
    layer.device = _device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    layer.opaque = YES;
    layer.presentsWithTransaction = NO;
    layer.allowsNextDrawableTimeout = YES;
    NSError *error = nil;
    NSURL *libraryURL = [NSBundle.mainBundle URLForResource:@"default" withExtension:@"metallib"];
    id<MTLLibrary> library = libraryURL ? [_device newLibraryWithURL:libraryURL error:&error] : nil;
    id<MTLFunction> vertex = [library newFunctionWithName:@"md_vertex"];
    id<MTLFunction> fragment = [library newFunctionWithName:@"md_fragment"];
    if (!vertex || !fragment) { [self refuse:error.localizedDescription ?: @"owned shader library unavailable"]; return; }
    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex;
    desc.fragmentFunction = fragment;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    _pipeline = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
    MTLTextureDescriptor *offscreen = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
        width:64 height:64 mipmapped:NO];
    offscreen.storageMode = MTLStorageModePrivate;
    offscreen.usage = MTLTextureUsageRenderTarget;
    _offscreen = [_device newTextureWithDescriptor:offscreen];
    _readback = [_device newBufferWithLength:64u * 256u options:MTLResourceStorageModeShared];
    if (!_pipeline || !_offscreen || !_readback) { [self refuse:error.localizedDescription ?: @"Metal resources unavailable"]; return; }
    /* Sentinel bytes must be overwritten by the completed GPU copy. */
    memset(_readback.contents, 0xA5, _readback.length);
    _drawable = [layer nextDrawable];
    if (!_drawable || _drawable.texture.pixelFormat != MTLPixelFormatBGRA8Unorm ||
        _drawable.texture.width != startGeometry.pixels_w || _drawable.texture.height != startGeometry.pixels_h) {
        [self refuse:@"drawable unavailable or dimensions differ from admitted geometry"]; return;
    }
    _command = [_queue commandBuffer];
    _command.label = @"Owned synthetic clear, triangle, readback, and present";
    if (!_command || ![self encodeTarget:_offscreen] || ![self encodeTarget:_drawable.texture]) {
        [self refuse:@"render encoding unavailable"]; return;
    }
    id<MTLBlitCommandEncoder> blit = [_command blitCommandEncoder];
    if (!blit) { [self refuse:@"readback encoding unavailable"]; return; }
    [blit copyFromTexture:_offscreen sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
        sourceSize:MTLSizeMake(64, 64, 1) toBuffer:_readback destinationOffset:0
        destinationBytesPerRow:256 destinationBytesPerImage:64u * 256u];
    [blit endEncoding];
    __weak MDController *weakOwner = self;
// MD_SIM_DEVICE_ONLY_BEGIN_presentation
#if !TARGET_OS_SIMULATOR
    [_drawable addPresentedHandler:^(id<MTLDrawable> presented) {
        double time = presented.presentedTime;
        dispatch_async(dispatch_get_main_queue(), ^{
            MDController *owner = weakOwner;
            if (!owner) return;
            md_presented(&owner->_policy, time);
            [owner publish:@"drawable-presented-callback" detail:@"System presented callback; not a human visibility assertion."];
        });
    }];
#endif
// MD_SIM_DEVICE_ONLY_END_presentation
    [_command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        BOOL completedOK = completed.status == MTLCommandBufferStatusCompleted && completed.error == nil;
        NSString *detail = completed.error.localizedDescription ?: @"GPU completion observed; independent offscreen samples checked.";
        dispatch_async(dispatch_get_main_queue(), ^{
            MDController *owner = weakOwner;
            if (!owner) return;
            /* No CPU read of shared output until a successful completion callback. */
            int pixelsOK = completedOK && md_pixels_match(owner->_readback.contents, owner->_readback.length);
            md_complete(&owner->_policy, completedOK, pixelsOK);
            [owner publish:@"command-completed-callback" detail:detail];
// MD_SIM_AUTOSTART_BEGIN_command_terminal
#if TARGET_OS_SIMULATOR
            if (owner->_simAutostartScheduled) {
                [owner publish:@"simulator-command-samples-terminal" detail:@"Genuine command completion observed; inspect GPU/sample result fields. Presentation callback unavailable on Simulator; no combined success claim."];
            }
#endif
// MD_SIM_AUTOSTART_END_command_terminal
        });
    }];
    MDGeometry commitGeometry = [self captureGeometry];
    if (!md_commit(&_policy, &commitGeometry)) {
        [self publish:@"commit-refused" detail:@"Foreground or geometry changed before commit."]; return;
    }
    /* No await, queue hop, or reentrant UIKit call between admission and commit. */
    [_command presentDrawable:_drawable];
    [_command commit];
    [self publish:@"committed" detail:@"Presentation scheduled; completion and actual presentation are still unproven."];
// MD_SIM_AUTOSTART_BEGIN_command_deadline
#if TARGET_OS_SIMULATOR
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (!self->_policy.completion_observed && !self->_policy.failed && !self->_policy.cancelled) {
            md_fail(&self->_policy);
            [self cancelForReason:@"Simulator command completion deadline expired; presentation observation is unavailable, resources retained."];
        }
    });
#endif
// MD_SIM_AUTOSTART_END_command_deadline
// MD_SIM_DEVICE_ONLY_BEGIN_presentation_deadline
#if !TARGET_OS_SIMULATOR
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (!md_success(&self->_policy) && !self->_policy.failed && !self->_policy.cancelled) {
            md_fail(&self->_policy);
            [self cancelForReason:@"Observation deadline expired; resources retained and late callbacks still recorded."];
        }
    });
#endif
// MD_SIM_DEVICE_ONLY_END_presentation_deadline
}
@end

@interface MDSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

/* One process lifetime, including after scene disconnection or uncertain GPU work. */
static MDController *retainedController;
static UIWindow *retainedWindow;
static BOOL sceneClaimed;

@implementation MDSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    (void)session; (void)options;
    if (![scene isKindOfClass:UIWindowScene.class] || sceneClaimed) return;
    sceneClaimed = YES;
    retainedController = [[MDController alloc] init];
    retainedWindow = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    retainedWindow.rootViewController = retainedController;
    self.window = retainedWindow;
    [retainedWindow makeKeyAndVisible];
}
- (void)sceneDidBecomeActive:(UIScene *)scene { (void)scene; [retainedController refreshAvailability]; }
- (void)sceneWillResignActive:(UIScene *)scene { (void)scene; [retainedController cancelForReason:@"scene resigned active"]; }
- (void)sceneDidEnterBackground:(UIScene *)scene { (void)scene; [retainedController cancelForReason:@"scene entered background"]; }
- (void)sceneDidDisconnect:(UIScene *)scene { (void)scene; [retainedController cancelForReason:@"scene disconnected"]; }
@end

@interface MDAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation MDAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    (void)application; (void)options;
    UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"Diagnostic" sessionRole:session.role];
    config.delegateClass = MDSceneDelegate.class;
    return config;
}
@end

int main(int argc, char **argv) {
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(MDAppDelegate.class)); }
}
