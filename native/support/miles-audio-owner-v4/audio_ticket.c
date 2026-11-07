#include "audio_ticket.h"

int ngp_miles_audio_borrow_v1(const NgpMilesAudioTicketV1 *ticket, NgpAudioWorkerContextsV1 *output)
{
    if (!ticket || !ticket->worker || !ticket->intent_epoch || !output) return NGP_AUDIO_WORKER_ARGUMENT;
    NgpAudioWorkerContextsV1 contexts = {0};
    NgpAudioWorkerReportV1 report = {.version = 1, .size = sizeof(report)};
    int status = ngp_audio_worker_v1_borrow(ticket->worker, &contexts, &report);
    if (status) return status;
    if (report.version != 1 || report.size != sizeof(report) || report.status ||
        report.phase != NGP_AUDIO_WORKER_READY || !report.worker_observation ||
        report.intent_epoch != ticket->intent_epoch || !report.voice_prepared ||
        !report.gate_enabled || report.ordinary_reserved != 32 || report.positional_reserved != 32 ||
        report.stream_reserved != 13 || !contexts.service || !contexts.ordinary ||
        !contexts.positional || !contexts.streams)
        return NGP_AUDIO_WORKER_RETAINED;
    *output = contexts;
    return 0;
}

int ngp_miles_audio_ready_v1(const NgpMilesAudioTicketV1 *ticket)
{
    NgpAudioWorkerContextsV1 contexts = {0};
    return ngp_miles_audio_borrow_v1(ticket, &contexts);
}

int ngp_miles_audio_bind_v1(NgpMilesAudioTicketV1 *output, NgpAudioWorkerRuntimeV1 *worker, uint64_t epoch)
{
    if (!output || output->worker || output->intent_epoch) return NGP_AUDIO_WORKER_ARGUMENT;
    NgpMilesAudioTicketV1 candidate = {.worker = worker, .intent_epoch = epoch};
    int status = ngp_miles_audio_ready_v1(&candidate);
    if (status) return status;
    *output = candidate;
    return 0;
}

int ngp_miles_audio_stop_ack_v1(const NgpMilesAudioTicketV1 *ticket)
{
    if (!ticket || !ticket->worker || !ticket->intent_epoch) return NGP_AUDIO_WORKER_ARGUMENT;
    NgpAudioWorkerStopReportV1 report = {.version = 1, .size = sizeof(report)};
    int status = ngp_audio_worker_v1_wait_stop(ticket->worker, &report);
    if (status) return status;
    if (report.version != 1 || report.size != sizeof(report) || report.status ||
        !report.worker_observation || report.intent_epoch != ticket->intent_epoch ||
        report.phase != NGP_AUDIO_WORKER_STOPPED_RETAINED ||
        !report.stop_consumed || !report.close_attempted || !report.close_ack ||
        !report.request_returned || report.broker_outcome != NGP_DEVICE_STOP_ACK_V1 ||
        report.first_worker_status)
        return NGP_AUDIO_WORKER_RETAINED;
    return 0;
}
