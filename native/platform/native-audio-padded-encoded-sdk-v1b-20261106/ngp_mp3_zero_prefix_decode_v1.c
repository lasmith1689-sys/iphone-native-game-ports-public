/* OWN wrapper; no implementation or policy changes in the actual decoder. */
#if !defined(__APPLE__)
#error "Requires actual Apple nonmain-worker predicate (Windows fixtures adapt explicitly)"
#endif
#include "ngp_mp3_zero_prefix_internal_v1.h"
#include <pthread.h>
int ngp_mp3_zero_prefix_v1_decode(const void*source,size_t bytes,NgpMemoryMp3V1**out,NgpMp3PaddingV1*padding){
 if(pthread_main_np()!=0)return NGP_MP3_WORKER_V1;
 int status=ngp_mp3_zero_prefix_v1_bounds(source,bytes,out,sizeof(*out),padding,sizeof(*padding));
 if(status)return status;
 if(*out)return NGP_MP3_BUSY_V1;
 NgpMp3PaddingV1 p;
 status=ngp_mp3_zero_prefix_v1_choose(source,bytes,&p);if(status)return status;
 status=ngp_memory_mp3_v1_decode((const uint8_t*)source+p.padding_bytes,p.suffix_bytes,out);
 if(status)return status;
 *padding=p;return 0;
}
