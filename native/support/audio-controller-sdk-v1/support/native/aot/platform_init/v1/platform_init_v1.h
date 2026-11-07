#ifndef COD_NATIVE_PLATFORM_INIT_V1_H
#define COD_NATIVE_PLATFORM_INIT_V1_H
#include <stdint.h>
#define COD_PLATFORM_ABI_V1 1u
#define COD_PLATFORM_TEXT_CAPACITY 96u
#define COD_INPUT_KEYBOARD 1u
#define COD_INPUT_POINTER 2u
#define COD_INPUT_TOUCH 4u
#define COD_INPUT_CONTROLLER 8u
#define COD_INPUT_KNOWN_CAPABILITIES 15u
/* New native service contract. No Windows structure, device handle or guest
 * address is a native64 pointer. Caller owns contexts and serializes lifecycle.
 * Readiness must require a real source, bound proven engine event consumer,
 * and current foreground ownership. start/restart recheck those conditions.
 * generation changes only after real lifecycle setup;0 means no live source.
 * poll drains only the provider's own native app events into its bound consumer.
 * Errors/unknown capability bits/unbound providers are never successful no-ops.
 */
typedef struct {
    void *context;
    int32_t (*ready)(void *,uint32_t *capabilities);
    int32_t (*start)(void *);
    int32_t (*restart)(void *);
    int32_t (*poll)(void *,uint64_t monotonic_ns);
    int32_t (*stop)(void *);
    uint64_t (*generation)(void *);
} CodNativeInputV1;
typedef struct {
    uint32_t abi,size;
    char platform[COD_PLATFORM_TEXT_CAPACITY];
    char cpu_identity[COD_PLATFORM_TEXT_CAPACITY];
    char profile[COD_PLATFORM_TEXT_CAPACITY];
    uint64_t physical_memory_bytes,monotonic_ns;
} CodNativePlatformSnapshotV1;
typedef struct {
    void *context;
    int32_t (*snapshot)(void *,CodNativePlatformSnapshotV1 *);
    int32_t (*monotonic)(void *,uint64_t *);
    CodNativeInputV1 input;
} CodNativePlatformServicesV1;
/* Engine bridge methods execute the actual registered native game functions.
 * Records remain guest32 coordinates. Cvar record slots/modified writes require
 * the separately verified original-source contract in this recovery version.
 */
typedef struct {
    void *context;
    int32_t (*cvar_get)(void *,const char *,const char *,uint32_t,uint32_t *);
    int32_t (*cvar_force_string)(void *,const char *,const char *);
    int32_t (*input_record)(void *,uint32_t original_slot,uint32_t guest_record);
    int32_t (*input_clear_modified)(void *,uint32_t guest_record);
    int32_t (*command)(void *,const char *,uint32_t guest_callback);
    int32_t (*text)(void *,const char *);
    int32_t (*remove_command)(void *,const char *);
} CodNativeEngineInitV1;
typedef struct {
    uint32_t abi,phase,input_capabilities,input_live;
    uint32_t input_records[9];
    uint64_t last_clock,input_generation;
    CodNativePlatformSnapshotV1 snapshot;
} CodNativePlatformStateV1;
enum {COD_PLATFORM_UNINITIALIZED=0,COD_PLATFORM_PREPARING=1,
      COD_PLATFORM_READY=2,COD_PLATFORM_STOPPED=3,COD_PLATFORM_FAILED=4};
enum {COD_PLATFORM_ERROR=-1,COD_PLATFORM_UNBOUND=-2,COD_PLATFORM_BAD_STATE=-3,
      COD_PLATFORM_PROVIDER_FAILED=-4,COD_PLATFORM_ENGINE_FAILED=-5};
int32_t cod_native_platform_init_v1(CodNativePlatformStateV1 *,const CodNativePlatformServicesV1 *,const CodNativeEngineInitV1 *);
int32_t cod_native_platform_restart_v1(CodNativePlatformStateV1 *,const CodNativePlatformServicesV1 *,const CodNativeEngineInitV1 *);
int32_t cod_native_platform_poll_v1(CodNativePlatformStateV1 *,const CodNativePlatformServicesV1 *);
int32_t cod_native_platform_stop_v1(CodNativePlatformStateV1 *,const CodNativePlatformServicesV1 *);
#endif
