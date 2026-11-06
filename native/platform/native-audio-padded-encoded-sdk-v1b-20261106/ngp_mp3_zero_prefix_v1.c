/* OWN finite prefix selection. Strict v1 remains a separate unchanged unit. */
#include "ngp_mp3_zero_prefix_internal_v1.h"
static int overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){
 return a<b+bn&&b<a+an;
}
int ngp_mp3_zero_prefix_v1_bounds(const void*source,size_t bytes,const void*a,size_t an,const void*b,size_t bn){
 if(!source||!a||!b||bytes<4)return NGP_MP3_INVALID_V1;
 if(bytes>NGP_MP3_MAX_BYTES_V1)return NGP_MP3_LIMIT_V1;
 uintptr_t s=(uintptr_t)source,x=(uintptr_t)a,y=(uintptr_t)b;
 if(s>UINTPTR_MAX-bytes||x>UINTPTR_MAX-an||y>UINTPTR_MAX-bn)return NGP_MP3_INVALID_V1;
 if(overlap(s,bytes,x,an)||overlap(s,bytes,y,bn)||overlap(x,an,y,bn))return NGP_MP3_INVALID_V1;
 return 0;
}
int ngp_mp3_zero_prefix_v1_choose(const void*source,size_t bytes,NgpMp3PaddingV1*out){
 const uint8_t*p=source;size_t at=0;uint32_t kind=NGP_MP3_PADDING_NONE_V1;
 /* Internal caller has checked the complete span and output disjointness. */
 if(p[0]==0||(p[0]==255&&p[1]==0)){
  kind=p[0]==0?NGP_MP3_PADDING_ZERO_V1:NGP_MP3_PADDING_FF_ZERO_V1;
  at=kind==NGP_MP3_PADDING_ZERO_V1?0u:1u;
  while(at<bytes&&at<=NGP_MP3_MAX_PADDING_V1&&p[at]==0)++at;
  if(at>NGP_MP3_MAX_PADDING_V1||bytes-at<4||p[at]!=255)return NGP_MP3_INVALID_V1;
 }
 NgpMp3PaddingV1 v={(uint32_t)bytes,(uint32_t)at,(uint32_t)(bytes-at),kind};
 *out=v;return 0;
}
int ngp_mp3_zero_prefix_v1_scan(const void*source,size_t bytes,NgpMp3EnvelopeV1*out,NgpMp3PaddingV1*padding){
 int status=ngp_mp3_zero_prefix_v1_bounds(source,bytes,out,sizeof(*out),padding,sizeof(*padding));
 if(status)return status;
 NgpMp3PaddingV1 p;NgpMp3EnvelopeV1 e;
 status=ngp_mp3_zero_prefix_v1_choose(source,bytes,&p);if(status)return status;
 status=ngp_mp3_envelope_v1_scan((const uint8_t*)source+p.padding_bytes,p.suffix_bytes,&e);
 if(status)return status;
 *out=e;*padding=p;return 0;
}
