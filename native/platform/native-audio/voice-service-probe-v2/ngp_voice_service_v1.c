/* OWN SOURCE CANDIDATE. No native/SDK result is implied by this file.
 * Link ONE frozen ngp_audio_shared_implementation_v1.c, never another MA TU. */
#if !defined(__APPLE__)
#error "Actual Darwin declarations required; no simulated native admission"
#endif
#include "ngp_voice_service_v1.h"
#include "ngp_voice_gain_balance_v2.h"
#include "ngp_audio_shared_profile_v1.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <float.h>
#define CLOSED 1u
#define INVALIDATED 2u
#define CLIP_MAX UINT64_C(67108864)
#define PCM_MAX UINT64_C(134217728)
#define HEAP_MAX UINT64_C(8388608)
#define WAIT_NS UINT64_C(2000000000)
#define WAIT_POLLS 1000000u
#define VOICES 77u
#define HDR_MAGIC UINT64_C(0x4e47505648454150)
_Static_assert(sizeof(float)==4&&FLT_RADIX==2&&FLT_MANT_DIG==24,"native IEEE Float32");
_Static_assert(sizeof(int16_t)==2&&sizeof(uintptr_t)==8,"native Apple64 PCM");
typedef union {max_align_t align;struct {uint64_t magic;size_t cost;} h;} HeapHeader;
typedef struct {
 ma_sound sound;ma_audio_buffer buffer;
 uint32_t sound_live,buffer_live,active,bound,kind,format,channels,rate,seek_pending,looping;
 uint64_t token,generation,frames,pcm_bytes;void *pcm;
 float pitch,volume,position[3],balance,cached_balance;NgpVoiceReportV1 cached;
} Voice;
struct NgpVoiceServiceV1 {
 NgpVoiceOutputV1 output;
 _Atomic uint32_t control,inflight,render_busy,owner_claimed,first_error;
 _Atomic int32_t first_library_error;
 _Atomic uint64_t render_sequence,allocator_violations,zero_callbacks;
 uint32_t phase,worker_op,was_open,engine_live;
 uint64_t next_handle,pcm_bytes,engine_bytes,engine_peak;
 void *pending_pcm;uint64_t pending_bytes;
 ma_engine engine;ma_allocation_callbacks alloc;Voice voices[VOICES];
 NgpVoiceReportV1 cached;
};
static _Atomic unsigned reserve_attempt;
static _Atomic(NgpVoiceServiceV1*) canonical;
static _Thread_local NgpVoiceServiceV1 *worker_context;
static const float own_silence[2]={0,0};

static int known(const NgpVoiceServiceV1*s){return s&&atomic_load_explicit(&canonical,memory_order_seq_cst)==s;}
static int report_valid(const NgpVoiceReportV1*r){return r&&r->version==1&&r->size==sizeof(*r);}
static void empty_report(NgpVoiceReportV1*r){memset(r,0,sizeof(*r));r->version=1;r->size=sizeof(*r);r->slot_index=UINT32_MAX;}
static int refusal(NgpVoiceReportV1*r,int code){if(report_valid(r)){empty_report(r);r->status=(uint32_t)code;}return code;}
static void mark_failure(NgpVoiceServiceV1*s,uint32_t code,int32_t library){
 atomic_fetch_or_explicit(&s->control,CLOSED|INVALIDATED,memory_order_seq_cst);
 uint32_t zero=0;atomic_compare_exchange_strong_explicit(&s->first_error,&zero,code,memory_order_seq_cst,memory_order_seq_cst);
 if(library){int32_t none=0;atomic_compare_exchange_strong_explicit(&s->first_library_error,&none,library,memory_order_seq_cst,memory_order_seq_cst);}
}
static uint32_t failure_code(NgpVoiceServiceV1*s){uint32_t c=atomic_load_explicit(&s->first_error,memory_order_seq_cst);return c?c:NGP_VOICE_CLOSED_V1;}
/* Finite atomic accounting, no wrap/no unbounded retry on callback paths. */
static int increment(NgpVoiceServiceV1*s,_Atomic uint64_t*counter){
 uint64_t n=atomic_load_explicit(counter,memory_order_seq_cst);
 for(unsigned i=0;i<4;i++){
  if(n==UINT64_MAX){mark_failure(s,NGP_VOICE_LIMIT_V1,0);return 0;}
  if(atomic_compare_exchange_strong_explicit(counter,&n,n+1,memory_order_seq_cst,memory_order_seq_cst))return 1;
 }
 mark_failure(s,NGP_VOICE_CONCURRENT_RENDER_V1,0);return 0;
}
static int allocator_in_render(NgpVoiceServiceV1*s){
 if(!atomic_load_explicit(&s->render_busy,memory_order_seq_cst))return 0;
 increment(s,&s->allocator_violations);mark_failure(s,NGP_VOICE_RENDER_V1,0);return 1;
}
static void *voice_malloc(size_t n,void*user){
 NgpVoiceServiceV1*s=user;if(allocator_in_render(s))return NULL;
 size_t bytes=n?n:1;if(bytes>SIZE_MAX-sizeof(HeapHeader)){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return NULL;}
 size_t cost=sizeof(HeapHeader)+bytes;
 if(s->engine_bytes>HEAP_MAX||cost>HEAP_MAX-s->engine_bytes){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return NULL;}
 HeapHeader*h=malloc(cost);if(!h){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return NULL;}
 h->h.magic=HDR_MAGIC;h->h.cost=cost;s->engine_bytes+=cost;if(s->engine_bytes>s->engine_peak)s->engine_peak=s->engine_bytes;return h+1;
}
static void voice_free(void*p,void*user){
 NgpVoiceServiceV1*s=user;if(allocator_in_render(s)||!p)return;
 HeapHeader*h=(HeapHeader*)p-1;
 /* Only this callback's live library allocations are valid input pointers. */
 if(h->h.magic!=HDR_MAGIC||h->h.cost>s->engine_bytes){mark_failure(s,NGP_VOICE_LIBRARY_V1,0);return;}
 s->engine_bytes-=h->h.cost;h->h.magic=0;free(h);
}
static void *voice_realloc(void*p,size_t n,void*user){
 NgpVoiceServiceV1*s=user;if(allocator_in_render(s))return NULL;if(!p)return voice_malloc(n,user);if(!n){voice_free(p,user);return NULL;}
 HeapHeader*h=(HeapHeader*)p-1;
 if(h->h.magic!=HDR_MAGIC||h->h.cost>s->engine_bytes||n>SIZE_MAX-sizeof(*h)){mark_failure(s,NGP_VOICE_LIBRARY_V1,0);return NULL;}
 size_t old=h->h.cost,cost=sizeof(*h)+n;uint64_t retained=s->engine_bytes-old;
 if(retained>HEAP_MAX||cost>HEAP_MAX-retained){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return NULL;}
 HeapHeader*next=realloc(h,cost);if(!next){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return NULL;}
 next->h.magic=HDR_MAGIC;next->h.cost=cost;s->engine_bytes=retained+cost;if(s->engine_bytes>s->engine_peak)s->engine_peak=s->engine_bytes;return next+1;
}
static int emit(NgpVoiceServiceV1*s,Voice*v,NgpVoiceReportV1*r,int code){
 *r=v?v->cached:s->cached;r->version=1;r->size=sizeof(*r);r->phase=s->phase;
 uint32_t first=atomic_load_explicit(&s->first_error,memory_order_seq_cst);if(first&&!code)code=(int)first;r->status=first?first:(uint32_t)code;
 r->library_status=atomic_load_explicit(&s->first_library_error,memory_order_seq_cst);
 r->owned_pcm_bytes=s->pcm_bytes;r->engine_heap_bytes=s->engine_bytes;
 r->render_allocator_violations=atomic_load_explicit(&s->allocator_violations,memory_order_seq_cst);r->zero_only_callbacks=atomic_load_explicit(&s->zero_callbacks,memory_order_seq_cst);return code;
}
static int owner(NgpVoiceServiceV1*s,NgpVoiceReportV1*r){
 if(!report_valid(r))return NGP_VOICE_ARGUMENT_V1;
 if(!s||worker_context!=s)return refusal(r,NGP_VOICE_OWNER_V1);
 if(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED){if(s->phase!=NGP_VOICE_CLOSED_RETAINED_V1)s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}
 if(s->worker_op||!(s->phase==NGP_VOICE_READY_GATED_V1||s->phase==NGP_VOICE_READY_OPEN_V1))return emit(s,NULL,r,NGP_VOICE_STATE_V1);
 return 0;
}
static Voice *lookup(NgpVoiceServiceV1*s,uint64_t token){if(!token)return NULL;for(unsigned i=0;i<VOICES;i++)if(s->voices[i].active&&s->voices[i].token==token)return&s->voices[i];return NULL;}
static int now_ns(uint64_t*out){
 struct timespec t;if(clock_gettime(CLOCK_MONOTONIC_RAW,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;
 if((uint64_t)t.tv_sec>(UINT64_MAX-(uint64_t)t.tv_nsec)/UINT64_C(1000000000))return 0;
 *out=(uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;return 1;
}
static int quiescent(NgpVoiceServiceV1*s){
 uint64_t start,now;if(!now_ns(&start)){mark_failure(s,NGP_VOICE_QUIESCENCE_V1,0);return 0;}
 for(unsigned i=0;i<WAIT_POLLS;i++){
  if(atomic_load_explicit(&s->inflight,memory_order_seq_cst)==0)return 1;
  if(!now_ns(&now)||now<start||now-start>=WAIT_NS)break;
 }
 mark_failure(s,NGP_VOICE_QUIESCENCE_V1,0);return 0;
}
static int begin(NgpVoiceServiceV1*s,NgpVoiceReportV1*r){
 int c=owner(s,r);if(c)return c;s->worker_op=1;
 uint32_t previous=atomic_fetch_or_explicit(&s->control,CLOSED,memory_order_seq_cst);s->was_open=previous==0;s->phase=NGP_VOICE_READY_GATED_V1;
 if(previous&INVALIDATED||!quiescent(s)||(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED)){
  s->phase=NGP_VOICE_FAILED_RETAINED_V1;s->worker_op=0;return emit(s,NULL,r,(int)failure_code(s));
 }
 return 0;
}
static int refresh(NgpVoiceServiceV1*s,Voice*v){
 NgpVoiceReportV1 r;empty_report(&r);r.slot_index=(uint32_t)(v-s->voices);r.kind=v->kind;r.bound=v->bound;r.handle=v->token;r.generation=v->generation;r.source_frames=v->bound?v->frames:0;r.cursor_is_source_domain=1;r.seek_may_be_pending=v->seek_pending;r.looping=v->looping;
 r.requested_pitch=v->pitch;r.requested_volume=v->volume;for(unsigned i=0;i<3;i++)r.position[i]=v->position[i];
 if(v->bound){ma_uint64 cursor=0;ma_result result=ma_sound_get_cursor_in_pcm_frames(&v->sound,&cursor);if(result!=MA_SUCCESS){mark_failure(s,NGP_VOICE_LIBRARY_V1,result);return 0;}r.source_cursor=cursor;r.playing=ma_sound_is_playing(&v->sound);r.looping=ma_sound_is_looping(&v->sound);}
 r.render_sequence=atomic_load_explicit(&s->render_sequence,memory_order_seq_cst);v->cached=r;v->cached_balance=v->balance;return 1;
}
static int finish(NgpVoiceServiceV1*s,Voice*v,NgpVoiceReportV1*r,ma_result result){
 if(result!=MA_SUCCESS)mark_failure(s,NGP_VOICE_LIBRARY_V1,result);
 if(!(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED)&&v)refresh(s,v);
 if(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED){s->phase=NGP_VOICE_FAILED_RETAINED_V1;s->worker_op=0;return emit(s,v,r,(int)failure_code(s));}
 s->worker_op=0;if(s->was_open){uint32_t expected=CLOSED;if(atomic_compare_exchange_strong_explicit(&s->control,&expected,0,memory_order_seq_cst,memory_order_seq_cst))s->phase=NGP_VOICE_READY_OPEN_V1;else{s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,v,r,(int)failure_code(s));}}
 return emit(s,v,r,0);
}
static int finite_float(float x){union {float f;uint32_t u;}v;v.f=x;return(v.u&UINT32_C(0x7f800000))!=UINT32_C(0x7f800000);}
static int coordinate(float x){return finite_float(x)&&x>=-1000000.0f&&x<=1000000.0f;}
static ma_result init_voice(NgpVoiceServiceV1*s,Voice*v,ma_format format,uint32_t channels,uint32_t rate,uint64_t frames,const void*pcm){
 ma_audio_buffer_config bc=ma_audio_buffer_config_init(format,channels,frames,pcm,&s->alloc);bc.sampleRate=rate;
 ma_result result=ma_audio_buffer_init(&bc,&v->buffer);if(result!=MA_SUCCESS)return result;v->buffer_live=1;
 ma_uint32 flags=v->kind==NGP_VOICE_POSITIONAL_V1?0:MA_SOUND_FLAG_NO_SPATIALIZATION;
 result=ma_sound_init_from_data_source(&s->engine,&v->buffer,flags,NULL,&v->sound);if(result!=MA_SUCCESS)return result;v->sound_live=1;
 result=ma_sound_stop(&v->sound);if(result!=MA_SUCCESS)return result;
 ma_sound_set_pitch(&v->sound,v->pitch);ma_sound_set_volume(&v->sound,v->volume);ma_sound_set_looping(&v->sound,v->looping);
 if(v->kind!=NGP_VOICE_POSITIONAL_V1){ma_sound_set_pan_mode(&v->sound,ma_pan_mode_balance);ma_sound_set_pan(&v->sound,v->balance);}
 if(v->kind==NGP_VOICE_POSITIONAL_V1)ma_sound_set_position(&v->sound,v->position[0],v->position[1],v->position[2]);return MA_SUCCESS;
}
static ma_result sentinel(NgpVoiceServiceV1*s,Voice*v){return init_voice(s,v,ma_format_f32,v->kind==NGP_VOICE_POSITIONAL_V1?1:s->output.channels,s->output.sample_rate,1,own_silence);}

int ngp_voice_service_v1_reserve(const NgpVoiceOutputV1*output,NgpVoiceServiceV1**out,NgpVoiceReportV1*r){
 if(!report_valid(r))return NGP_VOICE_ARGUMENT_V1;
 if(!output||!out||*out||output->version!=1||output->size!=sizeof(*output)||(output->sample_rate!=44100&&output->sample_rate!=48000)||(output->channels!=1&&output->channels!=2))return refusal(r,NGP_VOICE_ARGUMENT_V1);
 unsigned expected=0;if(!atomic_compare_exchange_strong_explicit(&reserve_attempt,&expected,1,memory_order_seq_cst,memory_order_seq_cst))return refusal(r,NGP_VOICE_STATE_V1);
 NgpVoiceServiceV1*s=calloc(1,sizeof(*s));if(!s)return refusal(r,NGP_VOICE_ALLOC_V1);
 s->output=*output;s->phase=NGP_VOICE_RESERVED_V1;s->next_handle=1;empty_report(&s->cached);
 atomic_init(&s->control,CLOSED);atomic_init(&s->inflight,0);atomic_init(&s->render_busy,0);atomic_init(&s->owner_claimed,0);atomic_init(&s->first_error,0);atomic_init(&s->first_library_error,0);atomic_init(&s->render_sequence,0);atomic_init(&s->allocator_violations,0);atomic_init(&s->zero_callbacks,0);
 atomic_store_explicit(&canonical,s,memory_order_seq_cst);*out=s;return emit(s,NULL,r,0);
}
int ngp_voice_service_v1_prepare(NgpVoiceServiceV1*s,NgpVoiceReportV1*r){
 if(!report_valid(r))return NGP_VOICE_ARGUMENT_V1;if(!known(s)||pthread_main_np())return refusal(r,NGP_VOICE_OWNER_V1);
 if(worker_context)return worker_context==s?emit(s,NULL,r,NGP_VOICE_STATE_V1):refusal(r,NGP_VOICE_OWNER_V1);
 uint32_t expected=0;if(!atomic_compare_exchange_strong_explicit(&s->owner_claimed,&expected,1,memory_order_seq_cst,memory_order_seq_cst))return refusal(r,NGP_VOICE_OWNER_V1);worker_context=s;
 if(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED){s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}
 s->phase=NGP_VOICE_PREPARING_V1;s->alloc.pUserData=s;s->alloc.onMalloc=voice_malloc;s->alloc.onRealloc=voice_realloc;s->alloc.onFree=voice_free;
 ma_engine_config ec=ma_engine_config_init();ec.noDevice=MA_TRUE;ec.channels=s->output.channels;ec.sampleRate=s->output.sample_rate;ec.listenerCount=1;ec.allocationCallbacks=s->alloc;ec.onProcess=NULL;
 ma_result result=ma_engine_init(&ec,&s->engine);if(result==MA_SUCCESS)s->engine_live=1;
 for(unsigned i=0;result==MA_SUCCESS&&i<VOICES;i++){
  if(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED)break;
  Voice*v=&s->voices[i];v->kind=i<32?NGP_VOICE_ORDINARY_V1:i<64?NGP_VOICE_POSITIONAL_V1:NGP_VOICE_STREAM_V1;v->pitch=1;v->volume=1;empty_report(&v->cached);v->cached.slot_index=i;v->cached.kind=v->kind;result=sentinel(s,v);
 }
 if(result!=MA_SUCCESS)mark_failure(s,NGP_VOICE_LIBRARY_V1,result);
 if(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED){s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}
 s->phase=NGP_VOICE_READY_GATED_V1;return emit(s,NULL,r,0);
}
int ngp_voice_service_v1_enable(NgpVoiceServiceV1*s,NgpVoiceReportV1*r){int c=owner(s,r);if(c)return c;uint32_t expected=CLOSED;if(atomic_compare_exchange_strong_explicit(&s->control,&expected,0,memory_order_seq_cst,memory_order_seq_cst)||expected==0){s->phase=NGP_VOICE_READY_OPEN_V1;return emit(s,NULL,r,0);}s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}
int ngp_voice_service_v1_acquire(NgpVoiceServiceV1*s,uint32_t kind,NgpVoiceHandleV1*out,NgpVoiceReportV1*r){
 int c=owner(s,r);if(c)return c;if(!out||*out)return emit(s,NULL,r,NGP_VOICE_ARGUMENT_V1);if(kind<1||kind>3)return emit(s,NULL,r,NGP_VOICE_KIND_V1);
 Voice*v=NULL;for(unsigned i=0;i<VOICES;i++)if(s->voices[i].kind==kind&&!s->voices[i].active&&s->voices[i].generation<UINT64_MAX){v=&s->voices[i];break;}
 if(!v||s->next_handle==UINT64_MAX)return emit(s,NULL,r,NGP_VOICE_LIMIT_V1);if((c=begin(s,r)))return c;
 v->generation++;v->token=s->next_handle++;v->active=1;int status=finish(s,v,r,MA_SUCCESS);if(!status)*out=v->token;return status;
}
static int target(NgpVoiceServiceV1*s,uint64_t token,NgpVoiceReportV1*r,Voice**out){int c=owner(s,r);if(c)return c;*out=lookup(s,token);return*out?0:emit(s,NULL,r,NGP_VOICE_HANDLE_V1);}
int ngp_voice_service_v1_bind_pcm(NgpVoiceServiceV1*s,NgpVoiceHandleV1 token,const NgpVoicePcmV1*pcm,NgpVoiceReportV1*r){
 Voice*v;int c=target(s,token,r,&v);if(c)return c;
 if(!pcm||pcm->version!=1||pcm->size!=sizeof(*pcm)||pcm->reserved||!pcm->bytes||pcm->channels<1||pcm->channels>2||pcm->sample_rate<8000||pcm->sample_rate>192000||!pcm->frame_count||pcm->format<1||pcm->format>3)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);
 uint64_t unit=pcm->format==1?1:pcm->format==2?2:4;
 if(pcm->frame_count>UINT64_MAX/(unit*pcm->channels)||pcm->byte_count!=pcm->frame_count*unit*pcm->channels||(uintptr_t)pcm->bytes>UINTPTR_MAX-pcm->byte_count)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);
 if(pcm->byte_count>CLIP_MAX||s->pcm_bytes>PCM_MAX||pcm->byte_count>PCM_MAX-s->pcm_bytes)return emit(s,v,r,NGP_VOICE_LIMIT_V1);
 if(pcm->format==3){const uint8_t*p=pcm->bytes;for(uint64_t i=0;i<pcm->byte_count;i+=4){float f;memcpy(&f,p+(size_t)i,4);if(!finite_float(f)||f< -1||f>1)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);}}
 if((c=begin(s,r)))return c;s->pending_pcm=malloc((size_t)pcm->byte_count);s->pending_bytes=pcm->byte_count;
 if(!s->pending_pcm){mark_failure(s,NGP_VOICE_ALLOC_V1,0);return finish(s,v,r,MA_SUCCESS);}
 s->pcm_bytes+=pcm->byte_count;memcpy(s->pending_pcm,pcm->bytes,(size_t)pcm->byte_count);
 if(v->sound_live){ma_sound_uninit(&v->sound);v->sound_live=0;}if(v->buffer_live){ma_audio_buffer_uninit(&v->buffer);v->buffer_live=0;}
 ma_format format=pcm->format==1?ma_format_u8:pcm->format==2?ma_format_s16:ma_format_f32;
 ma_result result=init_voice(s,v,format,pcm->channels,pcm->sample_rate,pcm->frame_count,s->pending_pcm);
 if(result==MA_SUCCESS&&!(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED)){
  free(v->pcm);s->pcm_bytes-=v->pcm_bytes;v->pcm=s->pending_pcm;v->pcm_bytes=s->pending_bytes;s->pending_pcm=NULL;s->pending_bytes=0;v->format=pcm->format;v->channels=pcm->channels;v->rate=pcm->sample_rate;v->frames=pcm->frame_count;v->bound=1;v->seek_pending=0;
 }
 return finish(s,v,r,result);
}
int ngp_voice_service_v1_play(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(!v->bound)return emit(s,v,r,NGP_VOICE_UNBOUND_V1);if((c=begin(s,r)))return c;return finish(s,v,r,ma_sound_start(&v->sound));}
int ngp_voice_service_v1_stop(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if((c=begin(s,r)))return c;return finish(s,v,r,ma_sound_stop(&v->sound));}
int ngp_voice_service_v1_seek(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,uint64_t frame,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(!v->bound)return emit(s,v,r,NGP_VOICE_UNBOUND_V1);if(frame>v->frames)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_result result=ma_sound_seek_to_pcm_frame(&v->sound,frame);if(result==MA_SUCCESS)v->seek_pending=1;return finish(s,v,r,result);}
int ngp_voice_service_v1_set_pitch(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,float pitch,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(!finite_float(pitch)||pitch<0.125f||pitch>8)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_sound_set_pitch(&v->sound,pitch);v->pitch=pitch;return finish(s,v,r,MA_SUCCESS);}
int ngp_voice_service_v1_set_volume(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,float volume,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(!finite_float(volume)||volume<0||volume>1)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_sound_set_volume(&v->sound,volume);v->volume=volume;return finish(s,v,r,MA_SUCCESS);}
int ngp_voice_service_v1_set_looping(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,uint32_t looping,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(looping>1)return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_sound_set_looping(&v->sound,looping);v->looping=looping;return finish(s,v,r,MA_SUCCESS);}
int ngp_voice_service_v1_set_position(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,float x,float y,float z,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if(v->kind!=NGP_VOICE_POSITIONAL_V1)return emit(s,v,r,NGP_VOICE_KIND_V1);if(!coordinate(x)||!coordinate(y)||!coordinate(z))return emit(s,v,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_sound_set_position(&v->sound,x,y,z);v->position[0]=x;v->position[1]=y;v->position[2]=z;return finish(s,v,r,MA_SUCCESS);}
int ngp_voice_service_v1_set_listener_position(NgpVoiceServiceV1*s,float x,float y,float z,NgpVoiceReportV1*r){int c=owner(s,r);if(c)return c;if(!coordinate(x)||!coordinate(y)||!coordinate(z))return emit(s,NULL,r,NGP_VOICE_ARGUMENT_V1);if((c=begin(s,r)))return c;ma_engine_listener_set_position(&s->engine,0,x,y,z);return finish(s,NULL,r,MA_SUCCESS);}
int ngp_voice_service_v1_snapshot(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if((c=begin(s,r)))return c;return finish(s,v,r,MA_SUCCESS);}
int ngp_voice_service_v1_release(NgpVoiceServiceV1*s,NgpVoiceHandleV1 t,NgpVoiceReportV1*r){Voice*v;int c=target(s,t,r,&v);if(c)return c;if((c=begin(s,r)))return c;if(v->sound_live){ma_sound_uninit(&v->sound);v->sound_live=0;}if(v->buffer_live){ma_audio_buffer_uninit(&v->buffer);v->buffer_live=0;}v->pitch=1;v->volume=1;v->looping=0;v->balance=0;for(unsigned i=0;i<3;i++)v->position[i]=0;ma_result result=sentinel(s,v);
 if(result==MA_SUCCESS&&!(atomic_load_explicit(&s->control,memory_order_seq_cst)&INVALIDATED)){free(v->pcm);s->pcm_bytes-=v->pcm_bytes;v->pcm=NULL;v->pcm_bytes=0;v->bound=0;v->frames=0;v->seek_pending=0;v->active=0;v->token=0;}return finish(s,v,r,result);}
int ngp_voice_service_v1_close(NgpVoiceServiceV1*s,NgpVoiceReportV1*r){int c=owner(s,r);if(c)return c;atomic_fetch_or_explicit(&s->control,CLOSED|INVALIDATED,memory_order_seq_cst);if(!quiescent(s)){s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}if(atomic_load_explicit(&s->first_error,memory_order_seq_cst)){s->phase=NGP_VOICE_FAILED_RETAINED_V1;return emit(s,NULL,r,(int)failure_code(s));}s->phase=NGP_VOICE_CLOSED_RETAINED_V1;return emit(s,NULL,r,0);}
int ngp_voice_service_v1_invalidate(NgpVoiceServiceV1*s,uint32_t reason){if(!known(s)||!(reason==NGP_VOICE_CANCELLED_V1||reason==NGP_VOICE_ROUTE_V1||reason==NGP_VOICE_LIFECYCLE_V1))return NGP_VOICE_ARGUMENT_V1;mark_failure(s,reason,0);return 0;}

static void zero_output(float*out,uint32_t samples){volatile float*p=out;for(uint32_t i=0;i<samples;i++)p[i]=0;}
static int zero_result(NgpVoiceServiceV1*s,int code){if(!increment(s,&s->zero_callbacks))return(int)failure_code(s);return code;}
int ngp_voice_service_v1_render(NgpVoiceServiceV1*s,float*out,uint32_t frames,uint32_t capacity){
 if(!known(s)||!out||!frames||frames>NGP_VOICE_MAX_RENDER_FRAMES_V1||capacity<frames||capacity>NGP_VOICE_MAX_RENDER_FRAMES_V1)return NGP_VOICE_ARGUMENT_V1;
 uint32_t samples=frames*s->output.channels;if((uintptr_t)out>UINTPTR_MAX-(size_t)samples*sizeof(float))return NGP_VOICE_ARGUMENT_V1;
 zero_output(out,samples);uint32_t control=atomic_load_explicit(&s->control,memory_order_seq_cst);
 if(control)return zero_result(s,control&INVALIDATED?(int)failure_code(s):NGP_VOICE_GATED_V1);
 uint32_t n=atomic_load_explicit(&s->inflight,memory_order_seq_cst);int held=0;
 for(unsigned i=0;i<4;i++){if(n==UINT32_MAX)break;if(atomic_compare_exchange_strong_explicit(&s->inflight,&n,n+1,memory_order_seq_cst,memory_order_seq_cst)){held=1;break;}}
 if(!held){mark_failure(s,NGP_VOICE_CONCURRENT_RENDER_V1,0);return zero_result(s,(int)failure_code(s));}
 control=atomic_load_explicit(&s->control,memory_order_seq_cst);
 if(control){atomic_fetch_sub_explicit(&s->inflight,1,memory_order_seq_cst);return zero_result(s,control&INVALIDATED?(int)failure_code(s):NGP_VOICE_GATED_V1);}
 uint32_t expected=0;
 if(!atomic_compare_exchange_strong_explicit(&s->render_busy,&expected,1,memory_order_seq_cst,memory_order_seq_cst)){mark_failure(s,NGP_VOICE_CONCURRENT_RENDER_V1,0);atomic_fetch_sub_explicit(&s->inflight,1,memory_order_seq_cst);return zero_result(s,(int)failure_code(s));}
 ma_uint64 got=0;ma_result result=ma_engine_read_pcm_frames(&s->engine,out,frames,&got);
 if(result!=MA_SUCCESS||got!=frames)mark_failure(s,NGP_VOICE_RENDER_V1,result);
 for(uint32_t i=0;i<samples;i++)if(!finite_float(out[i])){mark_failure(s,NGP_VOICE_RENDER_V1,0);break;}
 increment(s,&s->render_sequence);control=atomic_load_explicit(&s->control,memory_order_seq_cst);
 int status=control?(control&INVALIDATED?(int)failure_code(s):NGP_VOICE_GATED_V1):0;
 if(status)zero_output(out,samples);
 atomic_store_explicit(&s->render_busy,0,memory_order_seq_cst);atomic_fetch_sub_explicit(&s->inflight,1,memory_order_seq_cst);
 return status?zero_result(s,status):0;
}
static int gain_report_valid(const NgpVoiceGainBalanceReportV2*r){return r&&r->version==2&&r->size==sizeof(*r);}
static int gain_emit(NgpVoiceServiceV1*s,Voice*v,const NgpVoiceReportV1*inner,NgpVoiceGainBalanceReportV2*r,int code){
 r->version=2;r->size=sizeof(*r);r->voice=*inner;r->requested_linear_gain=0;r->requested_native_balance=0;r->balance_mode=0;r->output_channels=0;r->gain_observation_valid=0;
 if(s&&worker_context==s){
  r->output_channels=s->output.channels;
  if(v&&inner->slot_index==(uint32_t)(v-s->voices)&&inner->handle==v->token){r->requested_linear_gain=inner->requested_volume;r->requested_native_balance=v->cached_balance;r->gain_observation_valid=1;}
 }
 return code;
}
int ngp_voice_service_v2_set_gain_balance(NgpVoiceServiceV1*s,NgpVoiceHandleV1 token,float gain,float balance,NgpVoiceGainBalanceReportV2*r){
 if(!gain_report_valid(r))return NGP_VOICE_ARGUMENT_V1;
 NgpVoiceReportV1 inner;empty_report(&inner);Voice*v=NULL;int code=target(s,token,&inner,&v);
 if(code)return gain_emit(s,v,&inner,r,code);
 if(v->kind==NGP_VOICE_POSITIONAL_V1)return gain_emit(s,v,&inner,r,emit(s,v,&inner,NGP_VOICE_KIND_V1));
 if(s->output.channels!=2)return gain_emit(s,v,&inner,r,emit(s,v,&inner,NGP_VOICE_UNSUPPORTED_V2));
 if(!finite_float(gain)||gain<0||gain>16||!finite_float(balance)||balance< -1||balance>1)return gain_emit(s,v,&inner,r,emit(s,v,&inner,NGP_VOICE_ARGUMENT_V1));
 if((code=begin(s,&inner)))return gain_emit(s,v,&inner,r,code);
 ma_sound_set_pan_mode(&v->sound,ma_pan_mode_balance);ma_sound_set_volume(&v->sound,gain);ma_sound_set_pan(&v->sound,balance);v->volume=gain;v->balance=balance;
 code=finish(s,v,&inner,MA_SUCCESS);return gain_emit(s,v,&inner,r,code);
}
