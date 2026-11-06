/* Own native provider, not a legacy API or device readiness certificate. */
#include "ngp_positional_provider_v1.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t token;float source[3];} Loan;
struct NgpPositionalProviderV1 {
 pthread_t owner;NgpVoiceServiceV1*service;NgpVoiceTokensV1*pool;
 _Atomic uint32_t phase;uint32_t active;int32_t backend_status;
 float factor,listener[3];Loan loans[32];
};
static _Atomic(NgpPositionalProviderV1*) canonical;
static int valid_report(const NgpPositionalReportV1*r){return r&&r->version==1&&r->size==sizeof *r;}
static NgpVoiceReportV1 vr(void){NgpVoiceReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 tr(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int known(NgpPositionalProviderV1*s){return s&&atomic_load_explicit(&canonical,memory_order_acquire)==s;}
static int emit(NgpPositionalProviderV1*s,NgpPositionalReportV1*r,int code,int worker){
 NgpPositionalReportV1 v={0};v.version=1;v.size=sizeof v;v.status=(uint32_t)code;
 if(s){v.phase=atomic_load_explicit(&s->phase,memory_order_acquire);if(v.phase==NGP_POSITIONAL_LIVE)v.capacity=32;}
 if(worker){v.active=s->active;v.metres_per_unit=s->factor;v.backend_status=s->backend_status;v.worker_observation=1;}
 *r=v;return code;
}
static int owner(NgpPositionalProviderV1*s,NgpPositionalReportV1*r){
 if(!valid_report(r))return NGP_POSITIONAL_ARGUMENT;
 if(!known(s))return emit(NULL,r,NGP_POSITIONAL_ARGUMENT,0);
 if(!pthread_equal(s->owner,pthread_self()))return emit(s,r,NGP_POSITIONAL_OWNER,0);
 return 0;
}
static int live(NgpPositionalProviderV1*s,NgpPositionalReportV1*r){int c=owner(s,r);if(c)return c;return atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_POSITIONAL_LIVE?0:emit(s,r,NGP_POSITIONAL_STATE,1);}
static int failed(NgpPositionalProviderV1*s,NgpPositionalReportV1*r,int status){s->backend_status=status;atomic_store_explicit(&s->phase,NGP_POSITIONAL_FAILED,memory_order_release);return emit(s,r,NGP_POSITIONAL_BACKEND,1);}
static Loan*find(NgpPositionalProviderV1*s,uint32_t token){if(!token||(token&3u)!=2)return NULL;for(unsigned i=0;i<32;i++)if(s->loans[i].token==token)return&s->loans[i];return NULL;}
static int scaled(const float source[3],float factor,float out[3]){
 for(unsigned i=0;i<3;i++){double x=(double)source[i]*(double)factor;if(!isfinite(source[i])||!isfinite(x)||x< -1000000.0||x>1000000.0)return 0;out[i]=(float)x;}
 return 1;
}
static int native(NgpPositionalProviderV1*s,uint32_t token,uint64_t*out,NgpPositionalReportV1*r){NgpVoiceTokensReportV1 t=tr();int code=ngp_voice_tokens_v1_resolve(s->pool,token,out,&t);return code?failed(s,r,code):0;}
int ngp_positional_v1_open(NgpVoiceServiceV1*service,NgpPositionalProviderV1**out,NgpPositionalReportV1*r){
 if(!valid_report(r))return NGP_POSITIONAL_ARGUMENT;
 if(!service||!out||*out)return emit(NULL,r,NGP_POSITIONAL_ARGUMENT,0);
 if(atomic_load_explicit(&canonical,memory_order_acquire))return emit(NULL,r,NGP_POSITIONAL_STATE,0);
 NgpPositionalProviderV1*s=calloc(1,sizeof *s);if(!s)return emit(NULL,r,NGP_POSITIONAL_ALLOC,0);
 s->owner=pthread_self();s->service=service;s->factor=1;atomic_init(&s->phase,NGP_POSITIONAL_OPENING);
 NgpPositionalProviderV1*empty=NULL;if(!atomic_compare_exchange_strong_explicit(&canonical,&empty,s,memory_order_acq_rel,memory_order_acquire)){free(s);return emit(NULL,r,NGP_POSITIONAL_STATE,0);}
 *out=s;NgpVoiceTokensReportV1 t=tr();int code=ngp_voice_tokens_v1_reserve(2,&s->pool,&t);if(code)return failed(s,r,code);
 code=ngp_voice_tokens_v1_prepare(s->pool,service,&t);if(code||t.phase!=NGP_TOKENS_READY||t.capacity!=32||t.reserved_tracked!=32||t.loans_tracked)return failed(s,r,code?code:NGP_TOKENS_STATE);
 atomic_store_explicit(&s->phase,NGP_POSITIONAL_LIVE,memory_order_release);return emit(s,r,0,1);
}
int ngp_positional_v1_acquire(NgpPositionalProviderV1*s,uint32_t*out,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;if(!out||*out)return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);
 Loan*l=NULL;for(unsigned i=0;i<32;i++)if(!s->loans[i].token){l=&s->loans[i];break;}if(!l)return emit(s,r,NGP_POSITIONAL_LIMIT,1);
 NgpVoiceTokensReportV1 t=tr();uint32_t token=0;code=ngp_voice_tokens_v1_assign(s->pool,&token,&t);if(code==NGP_TOKENS_LIMIT)return emit(s,r,NGP_POSITIONAL_LIMIT,1);if(code||!token)return failed(s,r,code?code:NGP_TOKENS_TOKEN);
 l->token=token;memset(l->source,0,sizeof l->source);s->active++;*out=token;return emit(s,r,0,1);
}
int ngp_positional_v1_resolve(NgpPositionalProviderV1*s,uint32_t token,uint64_t*out,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;if(!out||*out)return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);if(!find(s,token))return emit(s,r,NGP_POSITIONAL_TOKEN,1);
 uint64_t h=0;if((code=native(s,token,&h,r)))return code;*out=h;return emit(s,r,0,1);
}
int ngp_positional_v1_release(NgpPositionalProviderV1*s,uint32_t token,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;Loan*l=find(s,token);if(!l)return emit(s,r,NGP_POSITIONAL_TOKEN,1);
 NgpVoiceTokensReportV1 t=tr();code=ngp_voice_tokens_v1_release(s->pool,token,&t);if(code)return failed(s,r,code);memset(l,0,sizeof *l);s->active--;return emit(s,r,0,1);
}
int ngp_positional_v1_set_position(NgpPositionalProviderV1*s,uint32_t token,float x,float y,float z,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;Loan*l=find(s,token);if(!l)return emit(s,r,NGP_POSITIONAL_TOKEN,1);
 float source[3]={x,y,z},metres[3];if(!scaled(source,s->factor,metres))return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);uint64_t h=0;if((code=native(s,token,&h,r)))return code;
 NgpVoiceReportV1 v=vr();code=ngp_voice_service_v1_set_position(s->service,h,metres[0],metres[1],metres[2],&v);if(code)return failed(s,r,code);memcpy(l->source,source,sizeof source);return emit(s,r,0,1);
}
int ngp_positional_v1_set_listener(NgpPositionalProviderV1*s,float x,float y,float z,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;float source[3]={x,y,z},metres[3];if(!scaled(source,s->factor,metres))return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);
 NgpVoiceReportV1 v=vr();code=ngp_voice_service_v1_set_listener_position(s->service,metres[0],metres[1],metres[2],&v);if(code)return failed(s,r,code);memcpy(s->listener,source,sizeof source);return emit(s,r,0,1);
}
int ngp_positional_v1_set_factor(NgpPositionalProviderV1*s,float factor,NgpPositionalReportV1*r){
 int code=live(s,r);if(code)return code;if(!isfinite(factor)||factor<=0)return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);
 float positions[32][3],listener[3];uint64_t handles[32]={0};
 /* Validate the complete prospective set before any snapshot/mixer mutation. */
 if(!scaled(s->listener,factor,listener))return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);
 for(unsigned i=0;i<32;i++)if(s->loans[i].token&&!scaled(s->loans[i].source,factor,positions[i]))return emit(s,r,NGP_POSITIONAL_ARGUMENT,1);
 for(unsigned i=0;i<32;i++)if(s->loans[i].token&&(code=native(s,s->loans[i].token,&handles[i],r)))return code;
 for(unsigned i=0;i<32;i++)if(s->loans[i].token){NgpVoiceReportV1 v=vr();code=ngp_voice_service_v1_set_position(s->service,handles[i],positions[i][0],positions[i][1],positions[i][2],&v);if(code)return failed(s,r,code);}
 NgpVoiceReportV1 v=vr();code=ngp_voice_service_v1_set_listener_position(s->service,listener[0],listener[1],listener[2],&v);if(code)return failed(s,r,code);
 s->factor=factor;return emit(s,r,0,1);
}
int ngp_positional_v1_close(NgpPositionalProviderV1*s,NgpPositionalReportV1*r){
 int code=owner(s,r);if(code)return code;uint32_t phase=atomic_load_explicit(&s->phase,memory_order_acquire);if(phase==NGP_POSITIONAL_CLOSED)return emit(s,r,0,1);if(phase!=NGP_POSITIONAL_LIVE)return emit(s,r,NGP_POSITIONAL_STATE,1);
 NgpVoiceTokensReportV1 t=tr();code=ngp_voice_tokens_v1_close(s->pool,&t);if(code)return failed(s,r,code);memset(s->loans,0,sizeof s->loans);s->active=0;atomic_store_explicit(&s->phase,NGP_POSITIONAL_CLOSED,memory_order_release);return emit(s,r,0,1);
}
