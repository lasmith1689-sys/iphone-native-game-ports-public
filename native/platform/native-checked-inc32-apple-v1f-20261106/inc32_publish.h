#ifndef PC64_INC32_PUBLISH_V1_H
#define PC64_INC32_PUBLISH_V1_H
#include "native_dispatch.h"
/* Admitted valid sole-owner CPU only, after actual RMW success. Pure five
 * defined flags, no CF/GPR/RIP/native memory/provider mutation. */
void pc64_inc32_publish_v1(CPU*,uint64_t returned_old);
#endif
