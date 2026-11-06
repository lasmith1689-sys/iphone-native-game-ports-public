#include "ngp_fls_environment_v1.h"
#include "native_dispatch.h"
uint32_t ngp_fls_environment_host_bytes_v1(void){return pc64_host_size();}
void*ngp_fls_environment_active_v1(void){return pc64_active_host();}
uint32_t ngp_fls_environment_depth_v1(void){return pc64_dispatch_depth();}
