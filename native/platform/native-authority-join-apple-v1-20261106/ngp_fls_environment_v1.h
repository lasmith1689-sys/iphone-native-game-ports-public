#ifndef NGP_FLS_ENVIRONMENT_V1_H
#define NGP_FLS_ENVIRONMENT_V1_H
#include <stdint.h>
#if defined(_WIN32)
#define NGP_FLS_API __declspec(dllexport)
#else
#define NGP_FLS_API __attribute__((visibility("default")))
#endif
enum {NGP_FLS_ENV_ARGUMENT=-1,NGP_FLS_ENV_FRAME=-304};
/* Required integration contract, deliberately UNBOUND in the production
 * archive. One authoritative environment supplies the actual live host size,
 * this retained worker's active host identity, and its observed entry depth.
 * These functions have no mutation, guest callbacks or fabricated success.
 * Host storage is valid/live/aligned/disjoint and remains immutable while
 * the provider is published. The same worker owns all mutation; no async
 * cancellation or dlclose. Depth is not a whole-producer drain certificate.
 * The standalone Mac fixture supplies only an explicitly synthetic TLS host
 * and depth. It does not establish an engine host, stack or frame binding. */
uint32_t ngp_fls_environment_host_bytes_v1(void);
void*ngp_fls_environment_active_v1(void);
uint32_t ngp_fls_environment_depth_v1(void);
#endif
