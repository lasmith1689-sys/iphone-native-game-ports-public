#ifndef PC64_INC32_FLAGS_V1_H
#define PC64_INC32_FLAGS_V1_H
#include <stdint.h>
typedef struct{uint64_t zf,sf,of,pf,af;} Pc64Inc32FlagsV1;
/* Pure five-flag result only. CF is deliberately not an output. */
void pc64_inc32_flags_v1(uint32_t old,Pc64Inc32FlagsV1*out);
#endif
