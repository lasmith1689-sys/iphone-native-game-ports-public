/* Own native catalog. Never link the controlled synthetic test backends here. */
#include "ngp_audio_catalog_v1.h"
#include <pthread.h>
#include <stdatomic.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct NgpAudioCatalogV1 {
 pthread_t owner;NgpAudioWorkerRuntimeV1*runtime;NgpAudioWorkerContextsV1 contexts;
 _Atomic uint32_t phase;uint32_t ordinary,positional;int32_t backend;float factor;
};
static _Atomic(NgpAudioCatalogV1*) canonical;
static const char provider_name[]="Native iPhone Stereo Positional";
#define PHASE(n)(1u<<(n))
static int valid(const NgpAudioCatalogReportV1*r){return r&&r->version==1&&r->size==sizeof *r;}
static int known(const NgpAudioCatalogV1*s){return s&&atomic_load_explicit(&canonical,memory_order_acquire)==s;}
static NgpAudioWorkerReportV1 wr(void){NgpAudioWorkerReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpPositionalReportV1 pr(void){NgpPositionalReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpVoiceTokensReportV1 tr(void){NgpVoiceTokensReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int emit(NgpAudioCatalogV1*s,NgpAudioCatalogReportV1*r,int code,int worker){
 NgpAudioCatalogReportV1 v={0};v.version=1;v.size=sizeof v;v.status=(uint32_t)code;
 if(s)v.phase=atomic_load_explicit(&s->phase,memory_order_acquire);
 if(worker){v.worker_observation=1;v.ordinary_loans_tracked=s->ordinary;v.positional_loans_tracked=s->positional;v.backend_status=s->backend;v.metres_per_unit=s->factor;}
 *r=v;return code;
}
static int failed(NgpAudioCatalogV1*s,NgpAudioCatalogReportV1*r,int own,int backend){
 s->backend=backend;atomic_store_explicit(&s->phase,NGP_CATALOG_FAILED_RETAINED,memory_order_release);
 (void)ngp_voice_service_v1_invalidate(s->contexts.service,NGP_VOICE_CANCELLED_V1);return emit(s,r,own,1);
}
static int owner(NgpAudioCatalogV1*s,NgpAudioCatalogReportV1*r){
 if(!valid(r))return NGP_CATALOG_ARGUMENT;
 if(!known(s))return emit(NULL,r,NGP_CATALOG_ARGUMENT,0);
 if(!pthread_equal(s->owner,pthread_self()))return emit(s,r,NGP_CATALOG_OWNER,0);
 return 0;
}
static int admit(NgpAudioCatalogV1*s,NgpAudioCatalogReportV1*r,unsigned phases){
 int code=owner(s,r);if(code)return code;
 uint32_t phase=atomic_load_explicit(&s->phase,memory_order_acquire);
 if(!(phases&PHASE(phase)))return emit(s,r,NGP_CATALOG_STATE,1);
 NgpAudioWorkerContextsV1 c={0};NgpAudioWorkerReportV1 w=wr();code=ngp_audio_worker_v1_borrow(s->runtime,&c,&w);
 if(code||!w.worker_observation||w.phase!=NGP_AUDIO_WORKER_READY||
  c.service!=s->contexts.service||c.ordinary!=s->contexts.ordinary||c.positional!=s->contexts.positional||c.streams!=s->contexts.streams)
  return failed(s,r,NGP_CATALOG_RUNTIME,code?code:NGP_AUDIO_WORKER_STATE);
 return 0;
}
int ngp_audio_catalog_v1_bind(NgpAudioWorkerRuntimeV1*runtime,NgpAudioCatalogV1**out,NgpAudioCatalogReportV1*r){
 if(!valid(r))return NGP_CATALOG_ARGUMENT;
 if(!runtime||!out||*out)return emit(NULL,r,NGP_CATALOG_ARGUMENT,0);
 if(atomic_load_explicit(&canonical,memory_order_acquire))return emit(NULL,r,NGP_CATALOG_STATE,0);
 NgpAudioWorkerContextsV1 c={0};NgpAudioWorkerReportV1 w=wr();int code=ngp_audio_worker_v1_borrow(runtime,&c,&w);
 if(code==NGP_AUDIO_WORKER_OWNER)return emit(NULL,r,NGP_CATALOG_OWNER,0);
 if(code||!w.worker_observation||w.phase!=NGP_AUDIO_WORKER_READY||!c.service||!c.ordinary||!c.positional||!c.streams||
  w.ordinary_reserved!=32||w.positional_reserved!=32||w.stream_reserved!=13||!w.voice_prepared||!w.gate_enabled)return emit(NULL,r,NGP_CATALOG_RUNTIME,0);
 NgpAudioCatalogV1*s=calloc(1,sizeof *s);if(!s)return emit(NULL,r,NGP_CATALOG_ALLOC,0);
 s->owner=pthread_self();s->runtime=runtime;s->contexts=c;s->factor=1;atomic_init(&s->phase,NGP_CATALOG_BOUND);
 NgpAudioCatalogV1*empty=NULL;if(!atomic_compare_exchange_strong_explicit(&canonical,&empty,s,memory_order_acq_rel,memory_order_acquire)){free(s);return emit(NULL,r,NGP_CATALOG_STATE,0);}
 *out=s;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_enumerate(NgpAudioCatalogV1*s,uint32_t*it,uint32_t*provider,char*name,size_t cap,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_BOUND)|PHASE(NGP_CATALOG_OPEN));if(code)return code;
 if(!it||!provider||!name||*it>1||(*it==0&&cap<sizeof provider_name))return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 if(*it==1){*it=0;*provider=0;return emit(s,r,0,1);}
 memcpy(name,provider_name,sizeof provider_name);*provider=4;*it=1;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_open_provider(NgpAudioCatalogV1*s,uint32_t provider,uint32_t*legacy,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_BOUND));if(code)return code;
 if(provider!=4)return emit(s,r,NGP_CATALOG_TOKEN,1);if(!legacy)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 NgpPositionalReportV1 p=pr();code=ngp_positional_v1_set_factor(s->contexts.positional,1,&p);
 if(code||p.phase!=NGP_POSITIONAL_LIVE||p.capacity!=32||p.active||!p.worker_observation||p.metres_per_unit!=1)
  return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_POSITIONAL_STATE);
 s->factor=1;atomic_store_explicit(&s->phase,NGP_CATALOG_OPEN,memory_order_release);*legacy=0;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_attribute(NgpAudioCatalogV1*s,uint32_t provider,const char*name,size_t len,uint32_t*out,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;
 if(provider!=4)return emit(s,r,NGP_CATALOG_TOKEN,1);if(!name||!out||len>255)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 static const char maximum[]="Maximum supported samples";
 *out=len==sizeof maximum-1&&!memcmp(name,maximum,sizeof maximum-1)?32:UINT32_MAX;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_set_distance_factor(NgpAudioCatalogV1*s,uint32_t provider,float factor,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;
 if(provider!=4)return emit(s,r,NGP_CATALOG_TOKEN,1);if(!isfinite(factor)||factor<=0)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 NgpPositionalReportV1 p=pr();code=ngp_positional_v1_set_factor(s->contexts.positional,factor,&p);
 if(code==NGP_POSITIONAL_ARGUMENT)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 if(code||p.phase!=NGP_POSITIONAL_LIVE||!p.worker_observation||p.metres_per_unit!=factor)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_POSITIONAL_STATE);
 s->factor=factor;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_acquire_ordinary(NgpAudioCatalogV1*s,uint32_t*out,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;if(!out||*out)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 uint32_t token=0;NgpVoiceTokensReportV1 t=tr();code=ngp_voice_tokens_v1_assign(s->contexts.ordinary,&token,&t);
 if(code==NGP_TOKENS_LIMIT&&!token)return emit(s,r,NGP_CATALOG_LIMIT,1);
 if(code||!token||(token&3u)!=1||!t.worker_observation||t.phase!=NGP_TOKENS_READY||t.loans_tracked!=s->ordinary+1)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_TOKENS_STATE);
 s->ordinary=t.loans_tracked;*out=token;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_acquire_positional(NgpAudioCatalogV1*s,uint32_t provider,uint32_t*out,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;if(provider!=4)return emit(s,r,NGP_CATALOG_TOKEN,1);if(!out||*out)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 uint32_t token=0;NgpPositionalReportV1 p=pr();code=ngp_positional_v1_acquire(s->contexts.positional,&token,&p);
 if(code==NGP_POSITIONAL_LIMIT&&!token)return emit(s,r,NGP_CATALOG_LIMIT,1);
 if(code||!token||(token&3u)!=2||!p.worker_observation||p.phase!=NGP_POSITIONAL_LIVE||p.active!=s->positional+1)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_POSITIONAL_STATE);
 s->positional=p.active;*out=token;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_resolve_voice(NgpAudioCatalogV1*s,uint32_t token,uint64_t*out,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;if(!out||*out)return emit(s,r,NGP_CATALOG_ARGUMENT,1);
 uint32_t kind=token&3u;uint64_t h=0;
 if(!token||(kind!=1&&kind!=2))return emit(s,r,NGP_CATALOG_TOKEN,1);
 if(kind==1){NgpVoiceTokensReportV1 t=tr();code=ngp_voice_tokens_v1_resolve(s->contexts.ordinary,token,&h,&t);if(code==NGP_TOKENS_TOKEN)return emit(s,r,NGP_CATALOG_TOKEN,1);}
 else{NgpPositionalReportV1 p=pr();code=ngp_positional_v1_resolve(s->contexts.positional,token,&h,&p);if(code==NGP_POSITIONAL_TOKEN)return emit(s,r,NGP_CATALOG_TOKEN,1);}
 if(code||!h)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_VOICE_HANDLE_V1);*out=h;return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_release_voice(NgpAudioCatalogV1*s,uint32_t token,NgpAudioCatalogReportV1*r){
 int code=admit(s,r,PHASE(NGP_CATALOG_OPEN)|PHASE(NGP_CATALOG_CLOSED));if(code)return code;uint32_t kind=token&3u;
 if(!token||(kind!=1&&kind!=2))return emit(s,r,NGP_CATALOG_TOKEN,1);
 if(kind==1){
  NgpVoiceTokensReportV1 t=tr();code=ngp_voice_tokens_v1_release(s->contexts.ordinary,token,&t);if(code==NGP_TOKENS_TOKEN)return emit(s,r,NGP_CATALOG_TOKEN,1);
  if(code||!s->ordinary||!t.worker_observation||t.phase!=NGP_TOKENS_READY||t.loans_tracked!=s->ordinary-1)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_TOKENS_STATE);
  s->ordinary=t.loans_tracked;
 }else{
  if(atomic_load_explicit(&s->phase,memory_order_acquire)!=NGP_CATALOG_OPEN)return emit(s,r,NGP_CATALOG_STATE,1);
  NgpPositionalReportV1 p=pr();code=ngp_positional_v1_release(s->contexts.positional,token,&p);if(code==NGP_POSITIONAL_TOKEN)return emit(s,r,NGP_CATALOG_TOKEN,1);
  if(code||!s->positional||!p.worker_observation||p.phase!=NGP_POSITIONAL_LIVE||p.active!=s->positional-1)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_POSITIONAL_STATE);
  s->positional=p.active;
 }
 return emit(s,r,0,1);
}
int ngp_audio_catalog_v1_close_provider(NgpAudioCatalogV1*s,uint32_t provider,NgpAudioCatalogReportV1*r){
 int code=owner(s,r);if(code)return code;if(provider!=4)return emit(s,r,NGP_CATALOG_TOKEN,1);
 if(atomic_load_explicit(&s->phase,memory_order_acquire)==NGP_CATALOG_CLOSED)return emit(s,r,0,1);
 code=admit(s,r,PHASE(NGP_CATALOG_OPEN));if(code)return code;
 NgpPositionalReportV1 p=pr();code=ngp_positional_v1_close(s->contexts.positional,&p);
 if(code||!p.worker_observation||p.phase!=NGP_POSITIONAL_CLOSED||p.active)return failed(s,r,NGP_CATALOG_BACKEND,code?code:NGP_POSITIONAL_STATE);
 s->positional=0;atomic_store_explicit(&s->phase,NGP_CATALOG_CLOSED,memory_order_release);return emit(s,r,0,1);
}
