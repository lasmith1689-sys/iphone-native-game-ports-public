#ifndef COD_INPUT_QUEUE_V1_H
#define COD_INPUT_QUEUE_V1_H
#include "../../../aot/platform_init/v1/platform_init_v1.h"
enum {COD_INPUT_ENGINE_ABI_V1=1,COD_INPUT_CLOCK_MONOTONIC_RAW_NS=1,
      COD_INPUT_QUEUE_CAPACITY=256,COD_INPUT_EVENT_KEY=1,COD_INPUT_EVENT_MOTION=3,COD_INPUT_TOUCH_BUTTON=200};
/* Native worker callbacks, separately bound to the actual recovered consumer.
 * No callback may wait for UIKit/main while the provider lifecycle lock is
 * held. The engine callback always supplies zero length/pointer payload words.
 * Clock conversion uses the same actual engine epoch and uint32 wrap policy. */
typedef struct {
 uint32_t abi,size;void *context;
 int32_t (*ready)(void *);
 int32_t (*event_time)(void *,uint32_t clock_kind,uint64_t receipt_ns,uint32_t *engine_time);
 int32_t (*enqueue)(void *,uint32_t time,uint32_t type,int32_t value,int32_t value2);
} CodInputEngineV1;
typedef struct {uint64_t ns;uint32_t type;int32_t value,value2;} CodInputEventV1;
/* All calls must be externally serialized. UIKit owns its NSLock; a native
 * test uses an explicitly synthetic source flag, never a fake UIKit source. */
typedef struct {
 CodInputEngineV1 engine;CodInputEventV1 events[COD_INPUT_QUEUE_CAPACITY];
 uint32_t head,count,source_ready,live,failed,engine_down;
 uint64_t epoch,generation,last_poll;
} CodInputQueueV1;
int32_t cod_input_queue_init_v1(CodInputQueueV1 *,CodInputEngineV1);
void cod_input_queue_source_v1(CodInputQueueV1 *,uint32_t actual_source_ready);
int32_t cod_input_queue_ready_v1(CodInputQueueV1 *,uint32_t *capabilities);
int32_t cod_input_queue_start_v1(CodInputQueueV1 *,uint32_t restart,uint64_t receipt_ns);
int32_t cod_input_queue_push_v1(CodInputQueueV1 *,CodInputEventV1);
int32_t cod_input_queue_poll_v1(CodInputQueueV1 *,uint64_t monotonic_ns);
int32_t cod_input_queue_stop_v1(CodInputQueueV1 *,uint64_t receipt_ns);
uint64_t cod_input_queue_generation_v1(const CodInputQueueV1 *);
#endif
