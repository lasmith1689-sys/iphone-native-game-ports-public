#ifndef COD_GL_HOST_CAPSULE_V1_H
#define COD_GL_HOST_CAPSULE_V1_H
#include "cod_gl_owned_profile.h"

typedef struct CodGLHostCapsuleV1 CodGLHostCapsuleV1;
enum {COD_GL_HOST_ABI_V1=1,COD_GL_HOST_OK_V1=0,COD_GL_HOST_ARGUMENT_V1=-1,
 COD_GL_HOST_STATE_V1=-2,COD_GL_HOST_EXCEPTION_V1=-3,COD_GL_HOST_BACKEND_V1=-4};
typedef struct {
 uint32_t abi,bytes,profile_kind;
 uintptr_t worker_identity,context_identity;
 uint64_t context_generation,drawable_generation;
 uint32_t framebuffer,color_renderbuffer,depth_stencil_renderbuffer;
 uint32_t width,height,color_format,depth_stencil_format;
 uint32_t red_bits,green_bits,blue_bits,alpha_bits,depth_bits,stencil_bits;
} CodGLHostFactsV1;

/* Capsule and worker persist for process lifetime, including failed construction
 * and uncertain work. Actual app owns its view/window and serialized lifecycle.
 * The ObjC factory takes an actual live instance of the exact derivative view. */
#ifdef __OBJC__
@class CodGLCompositionViewV1;
COD_GL_API int cod_gl_host_v1_create(CodGLCompositionViewV1 *,CodGLHostCapsuleV1 **);
#endif
COD_GL_API int cod_gl_host_v1_validate(CodGLHostCapsuleV1 *);
/* One engine transaction spans all memory snapshots, callbacks and completion.
 * Main lifecycle is denied while it is held; main can never claim ownership.
 * The engine worker must remain alive for the capsule lifetime: pthread_equal
 * is not proof of identity after a pthread exits and its ID is reused. */
COD_GL_API int cod_gl_host_v1_enter(CodGLHostCapsuleV1 *);
COD_GL_API int cod_gl_host_v1_leave(CodGLHostCapsuleV1 *,int completed_successfully);
COD_GL_API int cod_gl_host_v1_read_facts(CodGLHostCapsuleV1 *,CodGLHostFactsV1 *);
COD_GL_API int cod_gl_host_v1_present(CodGLHostCapsuleV1 *,uint64_t expected_drawable_generation);
COD_GL_API int cod_gl_host_v1_pause(CodGLHostCapsuleV1 *);
COD_GL_API int cod_gl_host_v1_resume(CodGLHostCapsuleV1 *);
COD_GL_API int cod_gl_host_v1_close(CodGLHostCapsuleV1 *);
/* Exact CodGL callback signatures. context is the capsule, never a guest value. */
COD_GL_API int cod_gl_host_v1_invoke(void *,CodGLNativeCall *,CodGLNativeResult *);
COD_GL_API int cod_gl_host_v1_read_unpack(void *,CodGLUnpackState *);
COD_GL_API int cod_gl_host_v1_read_client(void *,CodGLClientState *);
COD_GL_API int cod_gl_host_v1_read_owned(void *,uint32_t,CodGLOwnedStateV12 *);
COD_GL_API void *cod_gl_host_v1_allocate(void *,uint32_t,uint32_t);
COD_GL_API void cod_gl_host_v1_release(void *,void *);
#endif
