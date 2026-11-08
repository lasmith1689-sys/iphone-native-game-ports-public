#ifndef COD_PCM_CACHE_REGISTRY_V1_H
#define COD_PCM_CACHE_REGISTRY_V1_H
#include <stdint.h>
#include <stddef.h>
/* Native-owned metadata. Calls are serialized by the real engine worker. This
 * does not itself prove the observer hooks or guest/native stack closure. */
enum { COD_PCM_CACHE_SLOTS_V1=256, COD_PCM_ALIAS_SLOTS_V1=2048 };
typedef struct { uint64_t owner,generation,audio_intent_epoch; const void *worker; } CodPcmIdentityV1;
/* Persistent session identity. Each host operation separately validates the
 * current active loading entry; its incrementing epoch is not a cache lifetime. */
typedef struct {
 void *context;
 void *(*allocate)(void *,size_t);
 void (*release)(void *,void *,size_t);
} CodPcmAllocatorV1;
typedef struct {
 uint32_t cache,requested,words[9],published,kind;
 uint64_t hunk_epoch,serial;
 uint8_t *copy;
} CodPcmCacheEntryV1;
typedef struct { uint32_t alias,cache;uint64_t cache_serial,hunk_epoch; } CodPcmAliasV1;
typedef struct {
 uint32_t address,bytes,format,rate;
 const uint8_t *memory;
 uint64_t lease,cache_serial,hunk_epoch;
} CodPcmLeaseV1;
typedef struct {
 CodPcmIdentityV1 identity;CodPcmAllocatorV1 allocator;
 uint32_t ready,retained,backing_guest,backing_bytes,hunk_guest,capacity,high,low;
 uint8_t *backing_memory;
 uint64_t backing_serial,hunk_epoch,next_serial,next_lease,total_owned;
 uint32_t pending,reset_pending,busy,pending_requested,pending_alignment,pending_high,pending_low,pending_pc;
 uint32_t rewind_pending,rewind_high,rewind_low;
 uint32_t pending_kind,pending_words[9];
 uint64_t pending_serial,borrowed;
 uint32_t borrowed_index;
 CodPcmCacheEntryV1 caches[COD_PCM_CACHE_SLOTS_V1];
 CodPcmAliasV1 aliases[COD_PCM_ALIAS_SLOTS_V1];
} CodPcmRegistryV1;
int cod_pcm_registry_init_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,const CodPcmAllocatorV1 *);
/* Called only after the real malloc route returned from42A9E2, and after the
 * original aligned-base store. serial is issued by the native allocation
 * observer, never from guest metadata; all free/realloc routes must revoke it. */
int cod_pcm_registry_backing_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint64_t serial,
 uint32_t allocation,void *memory,uint32_t requested,uint32_t hunk,uint32_t capacity);
/* All42ACD0 calls must be observed, not just audio constructors. The before and
 * after halves surround the unchanged original allocator. Caller supplies the
 * actual original return PC and successful returned address. */
int cod_pcm_registry_alloc_before_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,
 uint32_t return_pc,uint32_t alignment,uint32_t requested,uint32_t high,uint32_t low);
typedef struct {
 uint32_t source[9],rate_limit,bits_limit,channels_limit,rate,format,capacity;
} CodPcmConvertedV1;
/* Only the private bounded445BBE constructor observer supplies this record.
 * Native metadata is checked and attached to one actual allocation serial. */
int cod_pcm_registry_converted_before_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,
 const CodPcmConvertedV1 *,uint32_t alignment,uint32_t requested,uint32_t high,uint32_t low);
int cod_pcm_registry_alloc_after_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,
 uint32_t result,uint32_t high,uint32_t low);
/* Constructor completion happens only after original445A30 finished its copy
 * and released its temporary file. cache must be a constructor-observed block.
 * The core copies it to native immutable storage and never exposes guest bytes. */
int cod_pcm_registry_publish_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t cache);
/* The private adapter proves the original alias store and owned table extent.
 * original site42C82D publishes a new cache;42C84E shares an existing one. */
int cod_pcm_registry_alias_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t site,uint32_t alias,uint32_t cache);
int cod_pcm_registry_acquire_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t alias,
 uint32_t current_cache,uint32_t current_high,uint32_t current_low,uint32_t address,uint32_t bytes,CodPcmLeaseV1 *);
/* Finish after the synchronous native backend copy. Failure is retained;
 * snapshots remain owned until the explicit retired-worker disposal. */
int cod_pcm_registry_finish_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,const CodPcmLeaseV1 *,int copied);
/* Before/after actual42ABF0. Revocation precedes original memory reuse. */
int cod_pcm_registry_reset_before_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *);
int cod_pcm_registry_reset_after_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t high,uint32_t low);
/* Before and immediately after the actual persistent-high rewind store.
 * Preserve only cache allocations wholly inside the still-live prefix. */
int cod_pcm_registry_rewind_before_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t high,uint32_t low,uint32_t new_high);
int cod_pcm_registry_rewind_after_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t high,uint32_t low);
/* Before actual42C980 or another proven alias table retirement. */
int cod_pcm_registry_retire_aliases_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t table,uint32_t rows);
/* Exact live native allocation extent, observed before its real free. */
int cod_pcm_registry_forget_alias_span_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *,uint32_t base,uint32_t bytes);
/* Called before backing free/realloc, owner retirement or shutdown. */
int cod_pcm_registry_retire_v1(CodPcmRegistryV1 *,const CodPcmIdentityV1 *);
/* Only after the real worker is no longer using this state. No active borrow. */
int cod_pcm_registry_dispose_v1(CodPcmRegistryV1 *);
#endif
