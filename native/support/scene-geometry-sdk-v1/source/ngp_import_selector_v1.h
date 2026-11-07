/* Own fixed completed-import selector. No resource or engine readiness. */
#ifndef NGP_IMPORT_SELECTOR_V1_H
#define NGP_IMPORT_SELECTOR_V1_H
#include <stdint.h>
#include "ngp_import_marker_v1.h"
enum {NGP_IMPORT_SELECT_OK=0,NGP_IMPORT_SELECT_ARGUMENT=-20201,
 NGP_IMPORT_SELECT_IO=-20202,NGP_IMPORT_SELECT_FILE=-20203,
 NGP_IMPORT_SELECT_CHANGED=-20204,NGP_IMPORT_SELECT_FORMAT=-20205,
 NGP_IMPORT_SELECT_BUDGET=-20206};
enum {NGP_IMPORT_SELECT_STAGE_ARGUMENT=1,NGP_IMPORT_SELECT_STAGE_ROOT=2,
 NGP_IMPORT_SELECT_STAGE_PATH=3,NGP_IMPORT_SELECT_STAGE_OPEN=4,
 NGP_IMPORT_SELECT_STAGE_READ=5,NGP_IMPORT_SELECT_STAGE_STABLE=6,
 NGP_IMPORT_SELECT_STAGE_CLOSE=7,NGP_IMPORT_SELECT_STAGE_FORMAT=8};
typedef struct {int32_t status,native_error;uint32_t stage,bytes_read;} NgpImportSelectReportV1;
/* Trusted live NativeCoD directory FD, immutable complete ancestors; no other
 * process/Files/AFC writer during selection/use. Fixed marker only, no retry.
 * Valid disjoint writable37-byte UUID output and optional report are required.
 * Refusal leaves UUID output untouched. Report is current-call diagnostic POD.
 * Marker FD is owned here; caller retains/closes root FD. No hard syscall-time
 * bound, content verification, resource borrow, cwd or engine authority. */
int32_t ngp_import_select_v1(int native_cod_directory_fd,char transaction[37],NgpImportSelectReportV1 *report);
#endif
