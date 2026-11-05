#ifndef MORDOR_LOCAL_COMPILE_DECLARATIONS_H
#define MORDOR_LOCAL_COMPILE_DECLARATIONS_H
/* Own compile declarations only, NOT an Apple SDK header or provider.
 * External functions remain unresolved; no successful service is fabricated. */
#include <stddef.h>
_Noreturn void abort(void);
void *memcpy(void *restrict,const void *restrict,size_t);
void *memset(void *,int,size_t);
int memcmp(const void *,const void *,size_t);
float sqrtf(float);
float nearbyintf(float);
double sqrt(double);
#endif
