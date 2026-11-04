#ifndef COD_INPUT_QUEUE_V2_H
#define COD_INPUT_QUEUE_V2_H
#include "../input-v1/cod_input_queue_v1.h"
/* ABI1 engine consumer is unchanged; v2 accepts its now-proved zero-time
 * source fallback. Old queue/provider implementation and tests stay intact. */
typedef CodInputQueueV1 CodInputQueueV2;
int32_t cod_input_queue_init_v2(CodInputQueueV2 *,CodInputEngineV1);
void cod_input_queue_source_v2(CodInputQueueV2 *,uint32_t actual_source_ready);
int32_t cod_input_queue_ready_v2(CodInputQueueV2 *,uint32_t *capabilities);
int32_t cod_input_queue_start_v2(CodInputQueueV2 *,uint32_t restart,uint64_t receipt_ns);
int32_t cod_input_queue_push_v2(CodInputQueueV2 *,CodInputEventV1);
int32_t cod_input_queue_poll_v2(CodInputQueueV2 *,uint64_t monotonic_ns);
int32_t cod_input_queue_stop_v2(CodInputQueueV2 *,uint64_t receipt_ns);
uint64_t cod_input_queue_generation_v2(const CodInputQueueV2 *);
#endif
