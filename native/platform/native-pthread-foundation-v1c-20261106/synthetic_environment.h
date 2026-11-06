#ifndef NGP_SYNTHETIC_ENVIRONMENT_V1_H
#define NGP_SYNTHETIC_ENVIRONMENT_V1_H
#include "ngp_fls_environment_v1.h"
/* Test-only retained identity object and TLS depth; no engine/guest binding.
 * All objects persist through process exit. Bind/mutate only outside this
 * synthetic depth. No OS, host-image or game functionality is implemented. */
int32_t ngp_synthetic_host_init_v1(void*,uint32_t);
void ngp_synthetic_bind_v1(void*);
int32_t ngp_synthetic_enter_v1(void);
int32_t ngp_synthetic_leave_v1(void);
#endif
