#ifndef PC64_NATIVE_ATOMIC_REQUIRED_V1_H
#define PC64_NATIVE_ATOMIC_REQUIRED_V1_H
#include "native_dispatch.h"
enum{PC64_INC_CONTEXT=-360,PC64_INC_WORKER=-361,PC64_INC_STATE=-362,
 PC64_INC_CPU=-363,PC64_INC_SITE=-364,PC64_INC_IMAGE=-365,PC64_INC_ALIAS=-366};
enum{PC64_INC_RAX=1,PC64_INC_RCX=2,PC64_INC_RDX=3,PC64_INC_RAX_15C=4};
typedef struct{uint64_t sourceVA,expectedRIP;uint32_t operandKind,byteCount;uint8_t originalBytes[8];}Pc64IncSiteV1;
/* Exact valid/live/disjoint native inputs. No public recovered addresses.
 * Context starts zeroed, is immutable after init except its owned atomic
 * phase, and persists through process exit. Host/CPU/source/native controls
 * remain live, valid and disjoint; only the retained creating worker mutates.
 * Init/bind/unbind require actual depth0. Apple rejects native main; Windows
 * nonmain role must be supplied by actual bootstrap construction. Warm one
 * linked-once worker tag here only; no clock or G1 dependency. No cancellation,
 * dlclose, async guest callbacks or concurrent CPU access. One active binding
 * per worker, no reset/rebind after terminal stop. No whole-producer drain.
 * The generic descriptor capacity is1..9.
 * Region metadata/code stay immutable. Registered RMW DWORDs have valid
 * native atomic-compatible lifetimes; ALL concurrent accesses obey that
 * contract. Registration excludes native stacks/control state. Source data
 * may live in the image, but never alias copied descriptors/opcode spans.
 * Required helper never returns on refusal: foreign/unbound/incorrect CPU
 * admission does not dereference or mutate CPU; after admitted actor+CPU a
 * source/host fault stamps only RIP then genuinely stops/discards. Success
 * changes exactly5 flags/one real checked DWORD; CF and408 other CPU bytes
 * are untouched. No source FLS/locale/heap/lock/record-ready admission. */
PC64_API uint32_t pc64_inc_binding_size_v1(void);
PC64_API uint32_t pc64_inc_binding_alignment_v1(void);
PC64_API int32_t pc64_inc_binding_init_v1(void*,uint32_t,void*,CPU*,const Pc64IncSiteV1*,uint32_t);
PC64_API int32_t pc64_inc_bind_v1(void*);
PC64_API int32_t pc64_inc_unbind_after_stop_v1(void);
PC64_API void pc64_required_inc32_v1(CPU*,uint64_t,uint64_t);
#endif
