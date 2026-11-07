/* Native CoD GL boundary. Host pointers never enter 32-bit guest registers. */
#ifndef COD_GL_DISPATCH_H
#define COD_GL_DISPATCH_H
#include <stdint.h>
#include <stdatomic.h>
#define COD_GL_BRIDGE_ABI_VERSION 12
#define cod_gl_init cod_gl_v12b_init
#define cod_gl_token cod_gl_v12b_token
#define cod_gl_lookup_guest cod_gl_v12b_lookup_guest
#define cod_gl_dispatch cod_gl_v12b_dispatch
#define cod_gl_ios_invoke cod_gl_v12b_ios_invoke
#define cod_gl4es_execute cod_gl_v12b_gl4es_execute
#define cod_gl4es_finish cod_gl_v12b_gl4es_finish
#define CodGLView CodGLViewV12B
#define CodGLRenderWorker CodGLRenderWorkerV12B
#if defined(__APPLE__)
#define COD_GL_API __attribute__((visibility("default")))
#else
#define COD_GL_API
#endif

enum { COD_GL_OK=0,COD_GL_UNKNOWN=1,COD_GL_MEMORY=2,COD_GL_ARGUMENT=3,
       COD_GL_BACKEND=4,COD_GL_CAPACITY=5 };
enum { COD_GL_READ=1,COD_GL_WRITE=2,COD_GL_MAX_RANGES=32,COD_GL_MAX_NAMES=4096 };
enum { COD_GL_MAX_STAGING_BYTES=64*1024*1024,COD_GL_STAGING_ALIGNMENT=16 };
enum CodGLFunction {
 COD_GL_CLEAR,COD_GL_CLEAR_COLOR,COD_GL_VIEWPORT,COD_GL_ENABLE,COD_GL_DISABLE,
 COD_GL_BLEND_FUNC,COD_GL_DEPTH_FUNC,COD_GL_DEPTH_MASK,COD_GL_CULL_FACE,COD_GL_FRONT_FACE,
 COD_GL_MATRIX_MODE,COD_GL_LOAD_IDENTITY,COD_GL_LOAD_MATRIX_F,COD_GL_PUSH_MATRIX,COD_GL_POP_MATRIX,
 COD_GL_BEGIN,COD_GL_END,COD_GL_COLOR_4F,COD_GL_TEX_COORD_2F,COD_GL_VERTEX_3F,
 COD_GL_GEN_TEXTURES,COD_GL_DELETE_TEXTURES,COD_GL_BIND_TEXTURE,COD_GL_TEX_PARAMETER_I,
 COD_GL_GET_ERROR,COD_GL_GET_STRING,COD_GL_PIXEL_STORE_I,COD_GL_TEX_IMAGE_2D,
 COD_GL_ENABLE_CLIENT_STATE,COD_GL_DISABLE_CLIENT_STATE,COD_GL_VERTEX_POINTER,
 COD_GL_COLOR_POINTER,COD_GL_TEX_COORD_POINTER,COD_GL_NORMAL_POINTER,COD_GL_DRAW_ARRAYS,
 COD_GL_DRAW_ELEMENTS,COD_GL_GET_INTEGER_V,COD_GL_CLEAR_DEPTH,
 COD_GL_LIGHT_MODEL_F,COD_GL_LIGHT_MODEL_FV,COD_GL_MATERIAL_F,COD_GL_MATERIAL_FV,
 COD_GL_TEX_ENV_I,COD_GL_SHADE_MODEL,COD_GL_POLYGON_MODE,
 COD_GL_ACTIVE_TEXTURE,COD_GL_CLIENT_ACTIVE_TEXTURE,
 COD_GL_FOG_I,COD_GL_HINT,COD_GL_FOG_F,COD_GL_COMPRESSED_TEX_IMAGE_2D,COD_GL_FUNCTION_COUNT
};
typedef struct { uint32_t address,bytes,access; } CodGLRange;
typedef struct {
 uint8_t *base; const CodGLRange *ranges; uint32_t range_count;
} CodGLMemory;
/* Synchronous native call packet; words preserve exact incoming argument bits.
 * All pointers in this packet are HOST pointers and never serialized to MEM. */
typedef struct {
 enum CodGLFunction function; uint32_t words[9]; float matrix[16];
 uint32_t names[COD_GL_MAX_NAMES]; const void *pixels;
 /* Exactly one native signed32 output for the three admitted scalar pnames.
  * Seeded from guest bytes, never an unchecked guest output pointer. */
 int32_t query_integer;
 _Alignas(8) double clear_depth;
 /* Native-owned synchronous lighting data. A failed/uncertain context must
  * retain this entire CodGL object until real completion/context teardown. */
 _Alignas(16) float fixed_vector[4];
 /* Only the explicit owned-profile constructor may authorize real public mip setters. */
 uint32_t owned_profile_kind,owned_profile_tail,owned_unpack_alignment;
 uintptr_t owned_texture_identity,owned_glstate_identity,owned_context_identity;
 uint64_t owned_context_generation;uint32_t owned_texture_name,owned_native_texture_name;
} CodGLNativeCall;
enum {COD_GL_PIXELS_NOT_CONSUMED_V12=0,COD_GL_PIXELS_UNCERTAIN_V12=1,COD_GL_PIXELS_COMPLETED_V12=2};
typedef struct { uint32_t word; const uint8_t *string; uint32_t pixel_effect; } CodGLNativeResult;
/* V9 success for a nonnull TexImage2D stage or DrawArrays/DrawElements confirms real native
 * GLES completion before returning. Production UIKit uses system glFinish,
 * without GL4ES error-routing mutation. Synthetic tests mark that distinction. */
typedef int (*CodGLInvoke)(void *host_context,CodGLNativeCall *,CodGLNativeResult *);
typedef struct {
 uint32_t alignment,row_length,skip_rows,skip_pixels;
 uint32_t in_begin,compiling,unpack_buffer_bound;
 uint32_t texture_options,texture_object_options;
} CodGLUnpackState;
/* Must synchronously snapshot the matching pinned backend's actual state on
 * its context-owning thread, without GL queries or mutation of GL error state. */
typedef int (*CodGLReadUnpack)(void *host_context,CodGLUnpackState *);

/* Actual private VAO state, never a requested-setter shadow. Pointer is native
 * host64 data and must be checked back into the explicit guest mapping before
 * every draw. kind0 denotes an unadmitted generic/secondary/fog attribute.
 * The private helper enumerates EVERY attribute, including unknown ones.
 * Existing GL4ES contexts/native VBO pointers are never implicitly trusted. */
enum {COD_GL_ARRAY_VERTEX=1,COD_GL_ARRAY_NORMAL=2,COD_GL_ARRAY_COLOR=3,
      COD_GL_ARRAY_TEXCOORD0=4,COD_GL_MAX_ATTRIBUTES=32,COD_GL_MAX_TEXTURE_UNITS=16};
typedef struct {
 uint32_t kind,enabled,size,type,stride,buffer_bound,divisor;
 uintptr_t pointer;
} CodGLArray;
typedef struct {
 uint32_t in_begin,compiling,array_buffer_bound,element_buffer_bound;
 uint32_t client_texture,attribute_count,locked;
 uint32_t list_active,list_pending;
 uint32_t server_texture; /* Actual texture.active, distinct from client. */
 uint32_t texture_unit_count,texture_coordinate_base;
 CodGLArray attributes[COD_GL_MAX_ATTRIBUTES];
} CodGLClientState;
typedef int (*CodGLReadClient)(void *host_context,CodGLClientState *);
typedef struct {
 void *context;
 void *(*allocate)(void *,uint32_t bytes,uint32_t alignment);
 void (*release)(void *,void *allocation);
} CodGLAllocator;
typedef struct {
 CodGLMemory memory; CodGLInvoke invoke; CodGLReadUnpack read_unpack; CodGLReadClient read_client; void *host_context;
 CodGLAllocator allocator;
 uint32_t scratch_address,scratch_bytes,scratch_used;
 uint32_t string_addresses[4],string_capacities[4];
 /* Owned storage avoids a large platform stack frame; atomic lease rejects
  * concurrent/reentrant entry before touching packet or backend state. */
 _Atomic uint32_t in_dispatch; CodGLNativeCall packet; CodGLClientState client;
 /* A failed backend may have consumed/queued an upload without confirming
  * completion. Retain this one bounded allocation and forbid further entry;
  * never reinitialize/reclaim this instance before real completion/process
  * teardown. Normal successful upload frees its own stage after native finish. */
 uint32_t failed,quarantined_bytes;void *quarantined_pixels;
 struct CodGLOwnedProfileV12 *owned;
} CodGL;

/* Base is a separately allocated native64 mapping; ranges are explicit mapped
 * guest32 intervals, never an assumption that the entire 4GiB is accessible.
 * Scratch must be a dedicated writable guest interval supplied by the owner.
 * Memory/range descriptors must remain valid and unchanged during calls. */
COD_GL_API int cod_gl_init(CodGL *,CodGLMemory,CodGLInvoke,CodGLReadUnpack,CodGLReadClient,void *,CodGLAllocator,uint32_t scratch,uint32_t bytes);
/* Context-free descriptor metadata only. Production guest procedure lookup
 * must use cod_gl_lookup_guest, which enforces the constructed profile. */
COD_GL_API uint32_t cod_gl_token(const char *exact_name);
COD_GL_API int cod_gl_lookup_guest(CodGL *,uint32_t guest_name,uint32_t *token);
/* CPU bytes match pinned regs.json: EAX[0..3], ESP[16..19]. AOT has already
 * pushed the guest return word. On success ESP pops4+RET-N; enum/pointer returns
 * enter EAX. Other register bytes remain untouched. No FP return is admitted.
 * Nonzero means fail/unknown; owner must trap, never report Win32 success. */
COD_GL_API int cod_gl_dispatch(CodGL *,uint32_t token,uint8_t *registers,uint32_t register_bytes);
#endif
