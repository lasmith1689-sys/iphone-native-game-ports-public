#include "yield_zero.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int32_t pc_yield_zero_v1(void){Sleep(0);return 0;}
