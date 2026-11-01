#include "ngp_pcm_resampler_v1.h"
static uint8_t source[4096],output[65536];
static NgpPcmResamplerV1 state;
static void store(uint8_t*p,int v){uint16_t u=(uint16_t)v;p[0]=(uint8_t)u;p[1]=(uint8_t)(u>>8);}
static int sample(const uint8_t*p){uint32_t u=(uint32_t)p[0]|((uint32_t)p[1]<<8);return u>=32768?(int)u-65536:(int)u;}
typedef struct {const int16_t *input,*expected;uint32_t frames,src,ic,dst,oc,count;} TestVector;
#include "vectors.inc"
static int same(const void *a,const void *b,size_t n){
 const uint8_t*p=a,*q=b;for(size_t i=0;i<n;i++)if(p[i]!=q[i])return 0;return 1;
}
static int vector_test(const TestVector*v,uint32_t partitioned){
 static const uint32_t caps[]={1,0,2,7,13};
 for(uint32_t k=0;k<v->frames*v->ic;k++)store(source+1+2*k,v->input[k]);
 if(ngp_pcm_resampler_v1_init(&state,source+1,v->frames*v->ic*2,v->frames,v->src,v->ic,v->dst,v->oc))return 1;
 NgpPcmResultV1 query=ngp_pcm_resampler_v1_render(&state,0,0);
 if(query.status||query.produced||query.remaining!=v->count||query.finished)return 2;
 uint32_t used=0;
 for(uint32_t iteration=0;iteration<10000&&used<v->count;iteration++){
  uint32_t capacity=partitioned?caps[iteration%5]:v->count;
  NgpPcmResultV1 r=ngp_pcm_resampler_v1_render(&state,output+1,capacity*v->oc*2);
  if(r.status||r.produced>capacity||r.produced>v->count-used||r.remaining!=v->count-used-r.produced)return 3;
  for(uint32_t k=0;k<r.produced*v->oc;k++)if(sample(output+1+k*2)!=v->expected[used*v->oc+k])return 4;
  used+=r.produced;if(r.finished!=(used==v->count))return 5;
 }
 if(used!=v->count)return 6;
 NgpPcmResultV1 end=ngp_pcm_resampler_v1_render(&state,output,8*v->oc*2);
 return end.status||end.produced||end.remaining||!end.finished?7:0;
}
static int invalid_tests(void){
 if(ngp_pcm_resampler_v1_init(&state,source,4,2,11025,1,48000,2))return 1;
 NgpPcmResamplerV1 before=state;
 if(!ngp_pcm_resampler_v1_init(&state,source,4,2,0,1,48000,2)||!same(&state,&before,sizeof(state)))return 2;
 if(!ngp_pcm_resampler_v1_init(&state,source,8,2,11025,2,48000,1)||!same(&state,&before,sizeof(state)))return 3;
 if(!ngp_pcm_resampler_v1_init(&state,source,4,2,11025,1,0,2)||!same(&state,&before,sizeof(state)))return 4;
 if(!ngp_pcm_resampler_v1_init(&state,source,3,2,11025,1,48000,2)||!same(&state,&before,sizeof(state)))return 5;
 if(!ngp_pcm_resampler_v1_init(&state,&state,4,2,11025,1,48000,2)||!same(&state,&before,sizeof(state)))return 6;
 if(!ngp_pcm_resampler_v1_init(&state,(void*)(UINTPTR_MAX-15),64,32,11025,1,48000,2)||!same(&state,&before,sizeof(state)))return 7;
 if(!ngp_pcm_resampler_v1_init(&state,source,NGP_PCM_MAX_BYTES_V1+4u,UINT32_MAX,11025,1,48000,2)||!same(&state,&before,sizeof(state)))return 8;
 void *pointers[]={source,&state,output,0,(void*)(UINTPTR_MAX-15),output};
 size_t sizes[]={4,4,3,4,64,NGP_PCM_MAX_BYTES_V1+4u};
 for(uint32_t k=0;k<6;k++){
  uint8_t src_copy[4],out_copy[64];
  for(uint32_t j=0;j<4;j++)src_copy[j]=source[j];
  for(uint32_t j=0;j<64;j++)out_copy[j]=output[j];
  NgpPcmResultV1 r=ngp_pcm_resampler_v1_render(&state,pointers[k],sizes[k]);
  if(r.status!=NGP_PCM_INVALID_V1||r.produced||!same(&state,&before,sizeof(state))||!same(source,src_copy,4)||!same(output,out_copy,64))return 9;
 }
 state.cursor=state.total+1;
 if(ngp_pcm_resampler_v1_render(&state,output,8).status!=NGP_PCM_INVALID_V1)return 10;
 return 0;
}
__declspec(dllexport) int fixture_pcm_model_tests(void){
 static const int expected[]={0,250,500,750,1000,1000,1000,1000};
 store(source,0);store(source+2,1000);
 if(ngp_pcm_resampler_v1_init(&state,source,4,2,11025,1,44100,1))return 101;
 NgpPcmResultV1 r=ngp_pcm_resampler_v1_render(&state,output,16);
 if(r.status||r.produced!=8||r.remaining||!r.finished)return 102;
 for(unsigned k=0;k<8;k++)if(sample(output+2*k)!=expected[k])return 103;
 for(uint32_t k=0;k<sizeof(vectors)/sizeof(vectors[0]);k++)for(uint32_t p=0;p<2;p++){
  int rc=vector_test(vectors+k,p);if(rc)return 1000+(int)(k*20+p*10)+(int)rc;
 }
 int rc=invalid_tests();if(rc)return 10000+rc;
 return 0;
}
