#include "ngp_pcm_resampler_v1.h"

static int valid_span(const void *p,size_t n){
 return p && n && (uintptr_t)p<=UINTPTR_MAX-(n-1);
}
static int overlaps(const void *p,size_t pn,const void *q,size_t qn){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
 if(!pn||!qn)return 0;
 return a<=b ? b-a<pn : a-b<qn;
}
static int format(uint32_t frames,uint32_t src,uint32_t ic,uint32_t dst,uint32_t oc,size_t bytes){
 uint64_t need=(uint64_t)frames*ic*2;
 return frames && (src==11025||src==22050||src==44100) &&
  (dst==44100||dst==48000) && (ic==1||ic==2) &&
  (oc==ic||(ic==1&&oc==2)) && need<=NGP_PCM_MAX_BYTES_V1 && need==bytes;
}
static uint64_t total_frames(uint32_t frames,uint32_t src,uint32_t dst){
 return ((uint64_t)frames*dst+src-1)/src;
}
int ngp_pcm_resampler_v1_init(NgpPcmResamplerV1 *s,const void *p,size_t bytes,
 uint32_t frames,uint32_t src,uint32_t ic,uint32_t dst,uint32_t oc){
 if(!valid_span(s,sizeof(*s))||!format(frames,src,ic,dst,oc,bytes)||
    !valid_span(p,bytes)||overlaps(s,sizeof(*s),p,bytes))return NGP_PCM_INVALID_V1;
 *s=(NgpPcmResamplerV1){1,frames,src,dst,ic,oc,p,bytes,0,total_frames(frames,src,dst)};
 return NGP_PCM_OK_V1;
}
static int32_t read_sample(const uint8_t *p){
 uint32_t v=(uint32_t)p[0]|((uint32_t)p[1]<<8);
 return v>=32768 ? (int32_t)v-65536 : (int32_t)v;
}
static void write_sample(uint8_t *p,int32_t v){
 uint16_t bits=(uint16_t)v;p[0]=(uint8_t)bits;p[1]=(uint8_t)(bits>>8);
}
NgpPcmResultV1 ngp_pcm_resampler_v1_render(NgpPcmResamplerV1 *s,void *output,size_t bytes){
 NgpPcmResultV1 invalid={NGP_PCM_INVALID_V1,0,0,0};
 if(!valid_span(s,sizeof(*s))||s->version!=1||
    !format(s->input_frames,s->source_rate,s->input_channels,s->destination_rate,s->output_channels,s->source_bytes)||
    !valid_span(s->source,s->source_bytes)||overlaps(s,sizeof(*s),s->source,s->source_bytes)||
    s->total!=total_frames(s->input_frames,s->source_rate,s->destination_rate)||s->cursor>s->total||
    bytes>NGP_PCM_MAX_BYTES_V1||bytes%(s->output_channels*2))return invalid;
 if(bytes&&(!valid_span(output,bytes)||overlaps(output,bytes,s,sizeof(*s))||
             overlaps(output,bytes,s->source,s->source_bytes)))return invalid;
 uint64_t remaining=s->total-s->cursor;
 uint32_t count=(uint32_t)(bytes/(s->output_channels*2));
 if((uint64_t)count>remaining)count=(uint32_t)remaining;
 uint8_t *out=output;
 for(uint32_t k=0;k<count;k++){
  uint64_t phase=(s->cursor+k)*s->source_rate;
  uint32_t i=(uint32_t)(phase/s->destination_rate),fraction=(uint32_t)(phase%s->destination_rate);
  uint32_t next=i+1<s->input_frames?i+1:i;
  for(uint32_t c=0;c<s->output_channels;c++){
   uint32_t channel=s->input_channels==1?0:c;
   int32_t a=read_sample(s->source+((size_t)i*s->input_channels+channel)*2);
   int32_t b=read_sample(s->source+((size_t)next*s->input_channels+channel)*2);
   int64_t mix=(int64_t)a*(s->destination_rate-fraction)+(int64_t)b*fraction;
   // Signed division truncates toward zero; explicit half offset gives ties away.
   int64_t rounded=mix>=0 ? (mix+s->destination_rate/2)/s->destination_rate :
                           -((-mix+s->destination_rate/2)/s->destination_rate);
   write_sample(out+((size_t)k*s->output_channels+c)*2,(int32_t)rounded);
  }
 }
 s->cursor+=count;remaining-=count;
 return (NgpPcmResultV1){NGP_PCM_OK_V1,count,remaining,remaining==0};
}
