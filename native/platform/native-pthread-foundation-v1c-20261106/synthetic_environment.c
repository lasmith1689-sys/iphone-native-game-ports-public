#include "synthetic_environment.h"
#include <stddef.h>
#include <stdlib.h>
typedef struct{uint64_t own_marker,retained_generation;} SyntheticHost;
static _Thread_local void*active;
static _Thread_local uint32_t depth;
uint32_t ngp_fls_environment_host_bytes_v1(void){return (uint32_t)sizeof(SyntheticHost);}
void*ngp_fls_environment_active_v1(void){return active;}
uint32_t ngp_fls_environment_depth_v1(void){return depth;}
int32_t ngp_synthetic_host_init_v1(void*p,uint32_t bytes){
 if(!p||((uintptr_t)p%_Alignof(SyntheticHost))||bytes!=sizeof(SyntheticHost))return -1;
 SyntheticHost*s=p;if(s->own_marker||s->retained_generation)return -1;
 s->own_marker=UINT64_C(0x4e4750464c5331);s->retained_generation=1;return 0;
}
void ngp_synthetic_bind_v1(void*p){if(depth)abort();active=p;}
int32_t ngp_synthetic_enter_v1(void){if(!active||depth)return -1;depth=1;return 0;}
int32_t ngp_synthetic_leave_v1(void){if(!active||depth!=1)return -1;depth=0;return 0;}
