#ifndef COD_NATIVE_HOST_V4_H
#define COD_NATIVE_HOST_V4_H
#include <stdint.h>
#include "platform_init_v1.h"
#include "cod_input_queue_v1.h"
struct CPU; /* Exact licensed rt.h definition has8960 bytes; never a host x86 CPU. */
#ifdef __cplusplus
extern "C" {
#endif
/* Bootstrap and teardown are serialized; caller joins render/worker threads and
 * releases all borrowed guest pointers before close. This data map is not a
 * sandbox for hostile machine states. No original x86 executable is invoked. */
uint8_t *rt_mem(void);
int32_t rt_owned_load_v3(uint32_t image_id,const uint8_t *native_bytes,uint32_t count);
int32_t rt_platform_configure(const char *explicit_app_profile);
int32_t rt_platform_bind_input(const CodNativeInputV1 *real_input);
const CodInputEngineV1 *rt_input_consumer_v1(void);
int32_t rt_owned_seal_v3(void);
/* Single fresh-engine entry: native policy + source-proven selected initializers
 * followed by generated common engine42FDE0. Return0 means this entry returned,
 * not that the game loaded. Every unknown input/UI/VM/import remains trapping. */
int32_t rt_native_common_v3(struct CPU*,const uint8_t *ascii_command,uint32_t count,int64_t budget);
int32_t rt_platform_poll(struct CPU*,int64_t budget);
int32_t rt_platform_stop_v4(struct CPU*,int64_t budget);
/* Normal shutdown executes known registered source callbacks. On failed/budget
 * stop, discard registrations instead, then close real streams/iterators. */
int rt_exit_run(struct CPU*,int64_t budget);
int rt_exit_discard(void);
int rt_stream_cleanup(uint32_t *closed);
int rt_find_cleanup(uint32_t *closed);
int32_t rt_owned_close_v3(void);
uint32_t rt_trap_addr(void);
uint32_t rt_native_boot_state_v3(uint32_t key);
uint32_t rt_owned_state_v3(uint32_t key);
#ifdef __cplusplus
}
#endif
#endif
