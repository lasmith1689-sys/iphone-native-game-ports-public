/* Own V4 diagnostic boot-trace recorder. Records only; it never decides,
 * retries, waits for another thread's logging, or reports engine readiness.
 * A recorder error never changes a caller's result: every entry is void or
 * returns only its own recorder status, and preserves errno. */
#ifndef NGP_BOOT_TRACE_V1_H
#define NGP_BOOT_TRACE_V1_H
#include <stddef.h>
#include <stdint.h>

enum {
 NGP_BOOT_TRACE_MAX_RECORDS=4096,      /* including the begin and cap records */
 NGP_BOOT_TRACE_MAX_RECORD_BYTES=192,  /* one line including its final LF */
 NGP_BOOT_TRACE_MAX_TOTAL_BYTES=524288,
 NGP_BOOT_TRACE_MAX_HEARTBEATS=120,    /* main-run-loop ticks, about 1 Hz */
 NGP_BOOT_TRACE_MAX_NAME=48,NGP_BOOT_TRACE_MAX_REASON=96,NGP_BOOT_TRACE_MAX_IDENT=16,
 NGP_BOOT_TRACE_WRITE_ATTEMPTS=8
};
enum {NGP_BOOT_TRACE_OK=0,NGP_BOOT_TRACE_ARGUMENT=-20301,NGP_BOOT_TRACE_STATE=-20302,
 NGP_BOOT_TRACE_IO=-20303,NGP_BOOT_TRACE_PATH=-20304};
enum {NGP_BOOT_TRACE_IDLE=0,NGP_BOOT_TRACE_ACTIVE=1,NGP_BOOT_TRACE_CAPPED=2,
 NGP_BOOT_TRACE_BROKEN=3,NGP_BOOT_TRACE_REFUSED=4};

/* Fixed event vocabulary. Names are emitted from the table in the .c file. */
#define NGP_BOOT_TRACE_EVENTS(X) \
 X(MAIN,"main") X(DID_FINISH,"didFinish") X(SCENE_CONFIG,"sceneConfig") \
 X(WILL_CONNECT,"willConnect") X(WINDOW,"window") X(HB_SCHEDULED,"hbScheduled") \
 X(ROOT_OPEN,"rootOpen") X(SELECT,"select") X(SETUP_UI,"setupUI") \
 X(CTL_INIT_ENTER,"ctlInitEnter") X(CTL_INIT_EXIT,"ctlInitExit") \
 X(ROOTVC_ENTER,"rootVCEnter") X(ROOTVC_EXIT,"rootVCExit") \
 X(KEYVIS_ENTER,"keyVisEnter") X(KEYVIS_EXIT,"keyVisExit") \
 X(SCENE_EXCEPTION,"sceneException") \
 X(ACTIVE_ENTER,"activeEnter") X(ACTIVE_EXIT,"activeExit") \
 X(RESIGN,"resign") X(BACKGROUND,"background") X(DISCONNECT,"disconnect") \
 X(COORD_UPDATE,"coordUpdate") X(APP_RESIGN,"appResign") \
 X(APP_BACKGROUND,"appBackground") X(APP_TERMINATE,"appTerminate") \
 X(HB_WINDOW,"hbWindow") X(HB_STATE,"hbState") X(HB_LABEL_FRAME,"hbLabelFrame") \
 X(HB_TEXT_EST,"hbTextEst") X(HB_FLAGS,"hbFlags") \
 X(LOADVIEW_ENTER,"loadViewEnter") X(LOADVIEW_EXIT,"loadViewExit") \
 X(START_ENTER,"startEnter") X(START_GATE,"startGate") X(SETUP_STAGE,"setupStage") \
 X(GL_CREATE_ENTER,"glCreateEnter") X(GL_CREATE_EXIT,"glCreateExit") \
 X(THREAD_START_ENTER,"threadStartEnter") X(THREAD_START_EXIT,"threadStartExit") \
 X(SETUP_FAILURE,"setupFailure") X(START_EXCEPTION,"startException") \
 X(CANCEL,"cancel") X(PAUSE_ENTER,"pauseEnter") X(PAUSE_EXIT,"pauseExit") \
 X(CONSUME,"consume") X(WORKER_ENTER,"workerEnter") X(WORKER_STAGE,"workerStage") \
 X(PUBLISH,"publish") X(WORKER_STOPPED,"workerStopped") \
 X(WORKER_EXCEPTION,"workerException") X(PARK,"park")
#define NGP_BOOT_TRACE_ENUM_ITEM(id,name) NGP_BT_##id,
enum {NGP_BT_NONE=0,NGP_BOOT_TRACE_EVENTS(NGP_BOOT_TRACE_ENUM_ITEM) NGP_BT_COUNT};
#undef NGP_BOOT_TRACE_ENUM_ITEM

/* Writer returns bytes written (0..count) or -1 with *error set. Clock returns
 * 0 and a monotonic value, or nonzero. Thread tag returns 'M' or 'W'. */
typedef long long (*NgpBootTraceWriteV1)(void *context,const void *bytes,size_t count,int *error);
typedef int (*NgpBootTraceClockV1)(void *context,uint64_t *nanoseconds);
typedef char (*NgpBootTraceThreadV1)(void *context);
typedef struct {
 NgpBootTraceWriteV1 write;void *write_context;
 NgpBootTraceClockV1 clock;void *clock_context;
 NgpBootTraceThreadV1 thread;void *thread_context;
 const char *bundle_version,*short_version; /* sanitized, capped at 16 */
 uint32_t expected_build;int64_t process_id;uint64_t run_id;
} NgpBootTraceConfigV1;
typedef struct {
 uint32_t state;uint64_t next_sequence,written_records,written_bytes;
 uint64_t dropped_records,write_failures,short_writes,interrupted_writes;
 uint64_t clock_failures,heartbeat_ticks;
} NgpBootTraceStatsV1;

/* Once per process. Writes the begin record; on its failure the recorder is
 * BROKEN and all later records are dropped (counted). */
int ngp_boot_trace_begin_v1(const NgpBootTraceConfigV1 *config);
/* Fixed event plus four integers. Never blocks on another thread's logging
 * except the shared mutex held for one bounded write sequence. */
void ngp_boot_trace_event_v1(uint32_t event,int64_t a,int64_t b,int64_t c,int64_t d);
/* Heartbeat record: counts toward NGP_BOOT_TRACE_MAX_HEARTBEATS when
 * first_of_tick is nonzero; later ticks are dropped and counted. */
void ngp_boot_trace_heartbeat_v1(uint32_t event,int first_of_tick,int64_t a,int64_t b,int64_t c,int64_t d);
/* Caught-exception text, capped and sanitized; path/credential-like tokens
 * are replaced. Lengths are byte counts of possibly non-NUL-terminated data. */
void ngp_boot_trace_exception_v1(uint32_t event,const char *name,size_t name_length,
                                 const char *reason,size_t reason_length);
void ngp_boot_trace_stats_v1(NgpBootTraceStatsV1 *out);
/* Exposed for tests: writes at most capacity-1 bytes plus NUL; returns length. */
size_t ngp_boot_trace_sanitize_v1(const char *in,size_t length,char *out,size_t capacity);
const char *ngp_boot_trace_event_name_v1(uint32_t event);

#if !defined(_WIN32)
/* Production Darwin opener: caches_path must be the app container's absolute
 * .../Library/Caches. Creates only Caches (if absent) and the fixed
 * NativeCoDBootTrace leaf, then truncates boot-trace-v1.log. Never touches
 * Application Support and never records the path. */
int ngp_boot_trace_open_posix_v1(const char *caches_path,const char *bundle_version,
                                 const char *short_version,uint32_t expected_build);
#endif
#ifdef NGP_BOOT_TRACE_TESTING
void ngp_boot_trace_reset_for_test_v1(void);
#endif
#endif
