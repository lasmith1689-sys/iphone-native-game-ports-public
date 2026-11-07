#ifndef COD_MILES_AUDIO_TICKET_V1
#define COD_MILES_AUDIO_TICKET_V1

#include "ngp_audio_worker_runtime_v1.h"

/* Private, caller-owned and retained on the actual persistent engine worker.
 * The caller binds this to the current host image, source CPU and exit scope. */
typedef struct {
    NgpAudioWorkerRuntimeV1 *worker;
    uint64_t intent_epoch;
} NgpMilesAudioTicketV1;

int ngp_miles_audio_bind_v1(NgpMilesAudioTicketV1 *, NgpAudioWorkerRuntimeV1 *, uint64_t);
int ngp_miles_audio_ready_v1(const NgpMilesAudioTicketV1 *);
int ngp_miles_audio_stop_ack_v1(const NgpMilesAudioTicketV1 *);

#endif
