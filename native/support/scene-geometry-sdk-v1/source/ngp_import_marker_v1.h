/* Own bounded selector protocol, not a game/resource readiness credential. */
#ifndef NGP_IMPORT_MARKER_V1_H
#define NGP_IMPORT_MARKER_V1_H
#include <stddef.h>
#include <stdint.h>
#define NGP_IMPORT_MARKER_PREFIX_V1 "{\"schema\":1,\"transaction\":\""
#define NGP_IMPORT_MARKER_SUFFIX_V1 "\"}\n"
enum {NGP_IMPORT_TRANSACTION_BYTES_V1=36,
 NGP_IMPORT_MARKER_BYTES_V1=sizeof(NGP_IMPORT_MARKER_PREFIX_V1)-1+36+sizeof(NGP_IMPORT_MARKER_SUFFIX_V1)-1};
/* Valid readable input and disjoint writable37-byte output. Refusal leaves
 * output untouched. No arbitrary-pointer, filesystem or ownership validation. */
int ngp_import_marker_decode_v1(const uint8_t *,size_t,char out_transaction[37]);
#endif
