#ifndef COD_GL_COMPOSITION_WORKER_V1_H
#define COD_GL_COMPOSITION_WORKER_V1_H
#import <UIKit/UIKit.h>
#include "cod_gl_dispatch.h"
#include "cod_gl_owned_profile.h"
#include "../cod_gl_host_v1.h"
#undef CodGLView
#undef CodGLRenderWorker
#undef cod_gl_ios_invoke
#define CodGLView CodGLCompositionViewV1
#define CodGLRenderWorker CodGLCompositionWorkerV1
#define cod_gl_ios_invoke cod_gl_composition_v1_raw_invoke

NS_ASSUME_NONNULL_BEGIN
/* Host app embeds a real view in its real window; this provides no app/game
 * entrypoint, fake HWND/HDC/HGLRC, or automatically successful Win32 API. */
COD_GL_API @interface CodGLView : UIView
@end

COD_GL_API @interface CodGLRenderWorker : NSObject
/* Main-thread creation, already attached nonzero drawable required. Exactly
 * one GL4ES context per process; close is permanent, pause/resume retains it. */
- (nullable instancetype)initWithView:(CodGLView *)view error:(NSError * _Nullable * _Nullable)error;
/* Explicit source/API opt-in: named native-CoD full-chain policy, not readiness.
 * Only the new factory establishes its public environment before GL4ES init. */
- (nullable instancetype)initWithView:(CodGLView *)view ownedTextureProfile:(uint32_t)namedProfile error:(NSError * _Nullable * _Nullable)error;
/* First invoke/present binds one engine pthread; other engine callers fail.
 * Synchronous queue execution keeps checked guest spans alive until return. */
- (int)invoke:(CodGLNativeCall *)call result:(CodGLNativeResult *)result;
- (int)readUnpack:(CodGLUnpackState *)state;
- (int)readClient:(CodGLClientState *)state;
- (int)readOwned:(CodGLOwnedStateV12 *)state requestedName:(uint32_t)name;
- (int)readFacts:(CodGLHostFactsV1 *)facts;
- (BOOL)present;
/* Main-thread lifecycle only. Host stops engine work at a completed frame before pause/close and
 * resumes it only after resume returns. No background GL execution admitted.
 * Stop and join enclosing bridge calls and all guest/staging users before close;
 * the worker admission count is not proof of that larger quiescence domain.
 * Busy public work refuses close before retirement and permits a later retry.
 * Once retirement begins, unknown/unfinished GL state, partial initialization or
 * teardown failure permanently refuses further work and retains uncertain
 * ownership until process exit. NO never authorizes freeing uncertain buffers.
 * YES completes the checked teardown block; it does not promise a thread join.
 * drawablePixelSize keeps its existing UI/owner policy; retirement returns zero. */
- (BOOL)pauseAndDrain;
- (BOOL)resume;
- (BOOL)close;
@property(nonatomic,readonly) CGSize drawablePixelSize;
@end
/* C callers use only the persistent capsule API. */
NS_ASSUME_NONNULL_END

#endif
