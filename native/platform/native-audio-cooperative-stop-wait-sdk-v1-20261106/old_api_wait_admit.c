/* Explicit TEST-only refusal shim. Existing old-API fixtures never invoke wait.
 * Production links the genuine ObjC bridge; no ready/success substitute. */
#include "ngp_audio_worker_runtime_v1.h"
int ngp_audio_worker_broker_wait_admit_v1(void*b,NgpVoiceServiceV1*s,uint64_t epoch){(void)b;(void)s;(void)epoch;return NGP_DEVICE_BROKER_ROUTE_V1;}
