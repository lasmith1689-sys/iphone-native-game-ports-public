#import "NGPMilesAudioOwnerV1.h"
#include <stdatomic.h>
#include <stdbool.h>

@implementation NGPMilesAudioOwnerV1 {
    NGPDeviceMixerIntentV1 *_intent;
    NGPDeviceMixerBrokerV1 *_broker;
    NSThread *_worker;
    uint64_t _intentEpoch;
    BOOL _attempted;
    _Atomic(bool) _published;
    _Atomic(bool) _cancelled;

    /* These fields are touched only by the retained engine thread. */
    NgpAudioWorkerRuntimeV1 *_runtime;
    NgpMilesAudioTicketV1 _ticket;
    int _workerFailure;
}

- (instancetype)initWithScene:(UIWindowScene *)scene
{
    if (!NSThread.isMainThread || !scene) return nil;
    self = [super init];
    if (self) {
        _intent = [[NGPDeviceMixerIntentV1 alloc] initWithScene:scene];
        if (!_intent) return nil;
        atomic_init(&_published, false);
        atomic_init(&_cancelled, false);
    }
    return self;
}

- (int)beginFromPlaybackActionForWorker:(NSThread *)worker
{
    if (!NSThread.isMainThread || !worker || worker == NSThread.mainThread ||
        worker.isExecuting || worker.isFinished || worker.isCancelled)
        return NGP_AUDIO_WORKER_OWNER;
    if (_attempted || atomic_load_explicit(&_cancelled, memory_order_acquire))
        return NGP_AUDIO_WORKER_STATE;
    _attempted = YES;
    @try {
        _intentEpoch = [_intent requestPlaybackIntent];
        if (!_intentEpoch) return NGP_AUDIO_WORKER_BROKER;
        _broker = [NGPDeviceMixerBrokerV1 openFromIntent:_intent];
        if (!_broker) {
            [self cancelOnMain];
            return NGP_AUDIO_WORKER_BROKER;
        }
        _worker = worker;
        atomic_store_explicit(&_published, true, memory_order_release);
        return 0;
    } @catch (NSException *exception) {
        (void)exception;
        [self cancelOnMain];
        return NGP_AUDIO_WORKER_BROKER;
    }
}

- (int)copyReadyTicketOnWorker:(NgpMilesAudioTicketV1 *)output
{
    if (!output || output->worker || output->intent_epoch) return NGP_AUDIO_WORKER_ARGUMENT;
    if (NSThread.isMainThread || !atomic_load_explicit(&_published, memory_order_acquire) ||
        NSThread.currentThread != _worker)
        return NGP_AUDIO_WORKER_OWNER;
    if (atomic_load_explicit(&_cancelled, memory_order_acquire)) return NGP_AUDIO_WORKER_RETAINED;
    if (_workerFailure) return _workerFailure;
    if (!_runtime) {
        NgpAudioWorkerReportV1 report = {.version = 1, .size = sizeof(report)};
        int status = ngp_audio_worker_v1_open((__bridge void *)_broker, &_runtime, &report);
        if (status == NGP_AUDIO_WORKER_PENDING && !_runtime) return status;
        if (status || !_runtime) {
            _workerFailure = status ? status : NGP_AUDIO_WORKER_STATE;
            return _workerFailure;
        }
        _workerFailure = ngp_miles_audio_bind_v1(&_ticket, _runtime, _intentEpoch);
        if (_workerFailure) return _workerFailure;
    }
    int status = ngp_miles_audio_ready_v1(&_ticket);
    if (status) return status;
    if (atomic_load_explicit(&_cancelled, memory_order_acquire)) return NGP_AUDIO_WORKER_RETAINED;
    *output = _ticket;
    return 0;
}

- (void)cancelOnMain
{
    if (!NSThread.isMainThread || atomic_exchange_explicit(&_cancelled, true, memory_order_acq_rel))
        return;
    @try {
        @try {
            [_intent clearPlaybackIntent];
        } @finally {
            [_broker requestStop];
        }
    } @catch (NSException *exception) {
        (void)exception;
        /* The retained worker can report an unacknowledged stop. */
    }
}

@end
