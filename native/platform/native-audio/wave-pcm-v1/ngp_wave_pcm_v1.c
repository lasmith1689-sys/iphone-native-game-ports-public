#include "ngp_wave_pcm_v1.h"
static uint32_t get16(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8);}
static uint32_t get32(const uint8_t*p){return get16(p)|(get16(p+2)<<16);}
static int id(const uint8_t*p,const char*s){return p[0]==(uint8_t)s[0]&&p[1]==(uint8_t)s[1]&&p[2]==(uint8_t)s[2]&&p[3]==(uint8_t)s[3];}
static int extent(const void*p,size_t n){return p&&n&&(uintptr_t)p<=UINTPTR_MAX-(n-1u);}
int ngp_wave_pcm_v1_parse(const void*source,size_t bytes,NgpWavePcmV1*out){
 if(bytes>NGP_WAVE_MAX_BYTES_V1)return NGP_WAVE_LIMIT_V1;
 if(bytes<12||!extent(source,bytes)||!extent(out,sizeof(*out)))return NGP_WAVE_INVALID_V1;
 if((uintptr_t)source<=(uintptr_t)out+sizeof(*out)-1u&&(uintptr_t)out<=(uintptr_t)source+bytes-1u)return NGP_WAVE_INVALID_V1;
 const uint8_t*p=source;
 if(!id(p,"RIFF")||!id(p+8,"WAVE"))return NGP_WAVE_UNSUPPORTED_V1;
 uint64_t end=(uint64_t)get32(p+4)+8u;
 if(end<12u||end>bytes)return NGP_WAVE_INVALID_V1;
 uint32_t position=12,seen_fmt=0,seen_data=0,chunks=0;
 NgpWavePcmV1 info={0};uint32_t byte_rate=0;
 info.container_bytes=(uint32_t)end;
 while(position<end){
  if(++chunks>NGP_WAVE_MAX_CHUNKS_V1)return NGP_WAVE_LIMIT_V1;
  if(end-position<8u)return NGP_WAVE_INVALID_V1;
  uint32_t count=get32(p+position+4);uint64_t data=(uint64_t)position+8u,next=data+count+(count&1u);
  if(next>end)return NGP_WAVE_INVALID_V1;
  if(id(p+position,"fmt ")){
   if(seen_fmt||count<16u)return NGP_WAVE_INVALID_V1;
   seen_fmt=1;info.format_tag=get16(p+data);info.channels=get16(p+data+2);
   info.sample_rate=get32(p+data+4);byte_rate=get32(p+data+8);
   info.block_align=get16(p+data+12);info.bits=get16(p+data+14);
  }else if(id(p+position,"data")){
   if(seen_data)return NGP_WAVE_INVALID_V1;
   seen_data=1;info.data_offset=(uint32_t)data;info.data_bytes=count;
  }
  position=(uint32_t)next;
 }
 if(!seen_fmt||!seen_data)return NGP_WAVE_INVALID_V1;
 if(info.format_tag!=1u||(info.channels!=1u&&info.channels!=2u)||(info.bits!=8u&&info.bits!=16u))return NGP_WAVE_UNSUPPORTED_V1;
 if(!info.sample_rate||info.sample_rate>192000u)return NGP_WAVE_UNSUPPORTED_V1;
 uint32_t align=info.channels*(info.bits/8u);
 if(info.block_align!=align||(uint64_t)info.sample_rate*align!=byte_rate||info.data_bytes%align)return NGP_WAVE_INVALID_V1;
 info.frames=info.data_bytes/align;info.sample_units=info.data_bytes/(info.bits/8u);
 *out=info;return NGP_WAVE_OK_V1;
}
