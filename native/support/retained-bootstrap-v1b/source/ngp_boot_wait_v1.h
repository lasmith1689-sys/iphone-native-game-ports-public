/* Own finite clock policy only. This is not an engine-entry/readiness API. */
#ifndef NGP_BOOT_WAIT_V1_H
#define NGP_BOOT_WAIT_V1_H
#include <stdint.h>
enum { NGP_BOOT_WAIT_PENDING=1, NGP_BOOT_WAIT_LIVE=0,
 NGP_BOOT_WAIT_ARGUMENT=-20001, NGP_BOOT_WAIT_CANCELLED=-20002,
 NGP_BOOT_WAIT_TIMEOUT=-20003, NGP_BOOT_WAIT_CLOCK=-20004 };
typedef struct { uint64_t last_ns, deadline_ns; int decision; } NgpBootWaitV1;
/* Private controller storage, initialized once per sole activation attempt.
 * Caller supplies observed monotonic timestamps, not wall-clock time. */
int ngp_boot_wait_begin_v1(NgpBootWaitV1 *, uint64_t now_ns, uint64_t timeout_ns);
/* checkpoint_status must be the real same-worker power-checkpoint result.
 * LIVE is a momentary observation only; common still checks actual power.
 * Any nonzero checkpoint may mean pending OR terminal; caller's finite deadline
 * and cancellation stop polling. This policy never authorizes a game call. */
int ngp_boot_wait_poll_v1(NgpBootWaitV1 *, uint64_t now_ns,
 int cancelled, int checkpoint_status);
#endif
