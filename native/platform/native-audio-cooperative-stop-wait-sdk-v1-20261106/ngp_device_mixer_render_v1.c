/* Own outer callback/scratch claim. No Apple/guest/codec/allocator calls. */
#include "ngp_device_mixer_render_v1.h"
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"native scalar atomics required");
_Static_assert(sizeof(float)==4,"Apple Float32 representation required");
static int span(const void*p,size_t n){return p&&n&&(uintptr_t)p<=UINTPTR_MAX-n;}
static int overlap(const void*a,size_t an,const void*b,size_t bn){
 return (uintptr_t)a<(uintptr_t)b+bn && (uintptr_t)b<(uintptr_t)a+an;
}
static int fault(NgpDeviceMixerRenderV1*s,int code){
 uint32_t expected=0;
 atomic_compare_exchange_strong_explicit(&s->failed,&expected,(uint32_t)code,memory_order_seq_cst,memory_order_seq_cst);
 if(s->service)(void)ngp_voice_service_v1_invalidate(s->service,NGP_VOICE_ROUTE_V1);
 return code;
}
static void zero(float *const*p,uint32_t count,uint32_t frames){
 for(uint32_t ch=0;ch<count;ch++)for(uint32_t i=0;i<frames;i++)p[ch][i]=0;
}
int ngp_device_mixer_render_v1(NgpDeviceMixerRenderV1*s,float *const*p,const size_t*bytes,
 const uint32_t*channels,uint32_t count,uint32_t frames,uint32_t*silence){
 if(!s||!silence)return NGP_DEVICE_MIXER_SHAPE_V1;
 *silence=1;
 if(!s->service||(s->channels!=1&&s->channels!=2))return fault(s,NGP_DEVICE_MIXER_SHAPE_V1);
 if(!frames)return NGP_DEVICE_MIXER_OK_V1;
 if(frames>NGP_VOICE_MAX_RENDER_FRAMES_V1)return fault(s,NGP_DEVICE_MIXER_FRAMES_V1);
 if(count!=s->channels||!p||!bytes||!channels||!span(s,sizeof(*s)))return fault(s,NGP_DEVICE_MIXER_SHAPE_V1);
 size_t needed=(size_t)frames*sizeof(float);
 for(uint32_t ch=0;ch<count;ch++){
  if(channels[ch]!=1||bytes[ch]<needed||!span(p[ch],needed)||((uintptr_t)p[ch]&(_Alignof(float)-1))||
     overlap(p[ch],needed,s,sizeof(*s)))return fault(s,NGP_DEVICE_MIXER_SHAPE_V1);
  for(uint32_t old=0;old<ch;old++)if(overlap(p[ch],needed,p[old],needed))return fault(s,NGP_DEVICE_MIXER_SHAPE_V1);
 }
 if(atomic_load_explicit(&s->failed,memory_order_seq_cst)){zero(p,count,frames);return NGP_DEVICE_MIXER_FAILED_V1;}
 uint32_t expected=0;
 if(!atomic_compare_exchange_strong_explicit(&s->busy,&expected,1,memory_order_seq_cst,memory_order_seq_cst)){
  zero(p,count,frames);return fault(s,NGP_DEVICE_MIXER_BUSY_V1);
 }
 /* No return between taking busy and its one release. A competing revocation
  * observed by the final SC check discards the copied PCM. Success linearizes
  * at that check; a later revoke cannot recall this block or success return. */
 int code=NGP_DEVICE_MIXER_OK_V1;
 if(atomic_load_explicit(&s->failed,memory_order_seq_cst))code=NGP_DEVICE_MIXER_FAILED_V1;
 else {
  int rc=ngp_voice_service_v1_render(s->service,s->scratch,frames,NGP_VOICE_MAX_RENDER_FRAMES_V1);
  if(rc==NGP_VOICE_GATED_V1)zero(p,count,frames);
  else if(rc!=NGP_VOICE_OK_V1){code=fault(s,NGP_DEVICE_MIXER_SERVICE_V1);zero(p,count,frames);}
  else {
   for(uint32_t ch=0;ch<count;ch++)for(uint32_t i=0;i<frames;i++)p[ch][i]=s->scratch[(size_t)i*count+ch];
   *silence=0;
  }
 }
 if(code||atomic_load_explicit(&s->failed,memory_order_seq_cst)){
  if(!code)code=NGP_DEVICE_MIXER_FAILED_V1;
  zero(p,count,frames);*silence=1;
 }
 atomic_store_explicit(&s->busy,0,memory_order_seq_cst);
 return code;
}
