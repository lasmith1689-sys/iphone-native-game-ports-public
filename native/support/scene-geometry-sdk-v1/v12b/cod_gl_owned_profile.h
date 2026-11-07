#ifndef COD_GL_OWNED_PROFILE_V12_H
#define COD_GL_OWNED_PROFILE_V12_H
#include "cod_gl_dispatch.h"
enum {COD_GL_PROFILE_NONE_V12=0,COD_GL_PROFILE_NATIVE_COD_FULL_CHAINS_V12=0x434f4431u};
enum {COD_GL_OWNED_MAX_TEXTURES_V12=4096,COD_GL_OWNED_LEVELS_V12=14};
enum {COD_GL_TEX_SHRINK_V12=1u,COD_GL_TEX_NPOT_V12=2u,COD_GL_TEX_STREAMED_V12=4u,
 COD_GL_TEX_COMPRESSED_V12=8u,COD_GL_TEX_RATIO_V12=16u,COD_GL_TEX_ADJUST_V12=32u,
 COD_GL_TEX_FBO_V12=64u,COD_GL_TEX_DEPTH_V12=128u,COD_GL_TEX_STENCIL_V12=256u,COD_GL_TEX_DATA_V12=512u};
enum {COD_GL_OPT_TEXCOPY_V12=1u,COD_GL_OPT_SHRINK_V12=2u,COD_GL_OPT_FORCE16_V12=4u,
 COD_GL_OPT_AVOID24_V12=8u,COD_GL_OPT_STREAM_V12=16u,COD_GL_OPT_NOLUM_V12=32u,
 COD_GL_OPT_FORCENPOT_V12=64u,COD_GL_OPT_TEXDUMP_V12=128u,COD_GL_OPT_NOERROR_V12=256u};
typedef struct {
 uintptr_t identity;uint32_t name,native_name,target;
 int32_t width,height,nwidth,nheight,base_level,max_level,valid;
 uint32_t mipmap_auto,mipmap_need,mipmap_done,min_filter,mag_filter,wrap_s,wrap_t,forbidden,sampler_override,enabled_targets;
} CodGLTextureFactV12;
typedef struct {
 uintptr_t context_identity,glstate_identity;uint64_t context_generation;
 uint32_t profile_kind,unit_count,active_unit,esversion,max_size;
 uint32_t in_begin,compiling,list_active,list_pending,pbo,alignment,row_length,skip_rows,skip_pixels;
 int32_t automipmap;uint32_t forbidden_globals,requested_exists;
 CodGLTextureFactV12 requested,units[COD_GL_MAX_TEXTURE_UNITS];
} CodGLOwnedStateV12;
/* Actual read-only private-layout snapshot on the live owned context worker.
 * request0 means default texture; lookup must not create/adopt an object. */
typedef int (*CodGLReadOwnedV12)(void*,uint32_t requested_name,CodGLOwnedStateV12*);
typedef struct {
 uintptr_t identity;uint64_t deletion_generation;
 uint32_t live,name,native_name,width,height,tail,provided_mask;
} CodGLOwnedTextureV12;
typedef struct CodGLOwnedProfileV12 {
 uint32_t kind;uintptr_t context_identity,glstate_identity;uint64_t context_generation,next_texture_generation;
 CodGLReadOwnedV12 read;CodGLOwnedStateV12 state;
 CodGLOwnedTextureV12 textures[COD_GL_OWNED_MAX_TEXTURES_V12];
 uint32_t pending_index,pending_new,pending_name,pending_width,pending_height,pending_level,pending_tail;
} CodGLOwnedProfileV12;
/* Explicit opt-in, separate from ordinary cod_gl_init. Caller retains profile
 * and bridge until real worker retirement. Profile is source/API policy, never
 * GPU readiness, allocation-success or a physical-device certificate. */
COD_GL_API int cod_gl_v12b_init_owned(CodGL*,CodGLOwnedProfileV12*,uint32_t named_profile,
 CodGLReadOwnedV12,CodGLMemory,CodGLInvoke,CodGLReadUnpack,CodGLReadClient,void*,CodGLAllocator,uint32_t,uint32_t);
int cod_gl_owned_v12b_before(CodGL*,CodGLNativeCall*);
int cod_gl_owned_v12b_after(CodGL*,CodGLNativeCall*);
int cod_gl_owned_v12b_verify_worker_call(const CodGLNativeCall*,const CodGLOwnedStateV12*);
#endif
