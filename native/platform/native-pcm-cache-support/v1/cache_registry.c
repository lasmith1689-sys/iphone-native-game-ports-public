#include "cache_registry.h"
#include <string.h>
#define CLIP_LIMIT (64u*1024u*1024u)
#define TOTAL_LIMIT (256u*1024u*1024u)
static int same(const CodPcmIdentityV1*a,const CodPcmIdentityV1*b){return a&&b&&a->owner==b->owner&&a->generation==b->generation&&a->audio_intent_epoch==b->audio_intent_epoch&&a->worker==b->worker;}
static int poison(CodPcmRegistryV1*s){if(s)s->retained=1;return -1;}
static int live(CodPcmRegistryV1*s,const CodPcmIdentityV1*i){return s&&s->ready&&!s->retained&&same(&s->identity,i);}
static int idle(CodPcmRegistryV1*s,const CodPcmIdentityV1*i){return live(s,i)&&s->backing_serial&&!s->borrowed&&!s->reset_pending&&!s->rewind_pending&&!s->busy;}
static int overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y|| (x<y+m&&y<x+n);}
static uint8_t*view(CodPcmRegistryV1*s,uint32_t p,uint32_t n){if(p<s->backing_guest||(uint64_t)p+n>(uint64_t)s->backing_guest+s->backing_bytes)return NULL;return s->backing_memory+(p-s->backing_guest);}
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static int find(CodPcmRegistryV1*s,uint32_t cache){for(unsigned k=0;k<COD_PCM_CACHE_SLOTS_V1;k++)if(s->caches[k].cache==cache&&s->caches[k].hunk_epoch==s->hunk_epoch)return (int)k;return -1;}
static int extent(CodPcmRegistryV1*s,uint32_t cache,uint32_t n){return cache>=s->hunk_guest&&(uint64_t)cache+n<=(uint64_t)s->hunk_guest+s->capacity&&view(s,cache,n);}
static int words_valid(const uint32_t*w,uint32_t cache,uint32_t requested){
 if(w[0]!=1||!w[2]||w[2]>CLIP_LIMIT||w[3]<8000||w[3]>192000||(w[4]!=8&&w[4]!=16)||(w[5]!=1&&w[5]!=2))return 0;
 unsigned frame=(w[4]/8)*w[5];
 return !(w[2]%frame)&&w[6]==w[2]/(w[4]/8)&&w[7]==frame&&(uint64_t)cache+36<=UINT32_MAX&&w[1]==cache+36&&w[8]==w[1]&&requested==((w[2]+39u)&~3u);
}
static void clear(CodPcmRegistryV1*s){
 s->busy=1;
 for(unsigned k=0;k<COD_PCM_CACHE_SLOTS_V1;k++){CodPcmCacheEntryV1*e=s->caches+k;if(e->copy)s->allocator.release(s->allocator.context,e->copy,e->words[2]);}
 memset(s->caches,0,sizeof s->caches);memset(s->aliases,0,sizeof s->aliases);s->total_owned=0;s->busy=0;
}
int cod_pcm_registry_init_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,const CodPcmAllocatorV1*a){
 if(!s||!i||!i->owner||!i->generation||!i->audio_intent_epoch||!i->worker||!a||!a->allocate||!a->release||s->ready||s->retained)return -1;
 /* The caller must supply zero-initialized native-owned state. */
 const uint8_t*b=(const uint8_t*)s;for(size_t k=0;k<sizeof *s;k++)if(b[k])return -1;
 s->identity=*i;s->allocator=*a;s->ready=1;s->hunk_epoch=1;return 0;
}
int cod_pcm_registry_backing_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint64_t serial,uint32_t base,void*memory,uint32_t requested,uint32_t hunk,uint32_t capacity){
 if(!live(s,i)||s->backing_serial||!serial||!base||!memory||!capacity||requested<31||(uint64_t)capacity+31!=requested||
   (uint64_t)base+requested>UINT64_C(0x100000000)||(uint64_t)base+31>UINT32_MAX||hunk!=((base+31u)&~31u)||
   (uint64_t)hunk+capacity>(uint64_t)base+requested||overlap(memory,requested,s,sizeof *s))return -1;
 s->backing_serial=serial;s->backing_guest=base;s->backing_memory=memory;s->backing_bytes=requested;s->hunk_guest=hunk;s->capacity=capacity;return 0;
}
static int converted_words(const CodPcmConvertedV1*p,uint32_t requested,uint32_t*w){
 if(!p||!w)return -1;const uint32_t*s=p->source;
 if(s[0]!=1||!s[1]||s[1]!=s[8]||!s[2]||s[2]>CLIP_LIMIT||(uint64_t)s[1]+s[2]>UINT64_C(0x100000000)||
    s[3]<8000||s[3]>192000||(s[4]!=8&&s[4]!=16)||(s[5]!=1&&s[5]!=2)||
    p->rate_limit<8000||p->rate_limit>192000||(p->bits_limit!=8&&p->bits_limit!=16)||(p->channels_limit!=1&&p->channels_limit!=2))return -1;
 uint32_t source_frame=(s[4]/8)*s[5];
 if(s[2]%source_frame||s[6]!=s[2]/(s[4]/8)||s[7]!=source_frame)return -1;
 if(s[3]<=p->rate_limit&&s[4]<=p->bits_limit&&s[5]<=p->channels_limit)return -1;
 uint32_t rate=s[3],units=s[6],bits=s[4]<p->bits_limit?s[4]:p->bits_limit,channels=s[5]<p->channels_limit?s[5]:p->channels_limit;
 while(rate>p->rate_limit){rate>>=1;units>>=1;}
 uint32_t format=(bits==16?1u:0u)+(channels==2?2u:0u);
 if(rate<8000||p->rate!=rate||p->format!=format)return -1;
 uint64_t numerator=(uint64_t)(s[2]/source_frame)*rate,frames=numerator/s[3],remainder=numerator%s[3];
 if(remainder*2>s[3]||(remainder*2==s[3]&&(frames&1)))frames++;
 uint64_t capacity=frames*(bits/8)*channels+256;
 if(capacity>CLIP_LIMIT||p->capacity!=capacity||requested!=((capacity+39)&~UINT64_C(3)))return -1;
 uint32_t expected[9]={1,0,(uint32_t)capacity,rate,bits,channels,units,s[7],0};memcpy(w,expected,sizeof expected);return 0;
}
static int alloc_before(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t pc,uint32_t align,uint32_t n,uint32_t high,uint32_t low,const CodPcmConvertedV1*converted){
 if(!idle(s,i)||s->pending||!pc||!align||(align&(align-1))||align>4096||!n||low>s->capacity||high>s->capacity-low)return -1;
 if(high!=s->high)return poison(s);
 uint32_t expected[9]={0};
 if(pc==0x445bbeu){if(!converted||converted_words(converted,n,expected))return -1;}
 else if(converted)return -1;
 uint64_t end=((uint64_t)high+n+align-1)&~((uint64_t)align-1);
 if(end>s->capacity-low||s->next_serial==UINT64_MAX)return -1;
 if(pc==0x445b20u||pc==0x445bbeu){unsigned k;for(k=0;k<COD_PCM_CACHE_SLOTS_V1;k++)if(!s->caches[k].cache)break;if(k==COD_PCM_CACHE_SLOTS_V1||align!=32||n<40||n>CLIP_LIMIT+39u)return -1;}
 s->pending=1;s->pending_pc=pc;s->pending_requested=n;s->pending_alignment=align;s->pending_high=(uint32_t)end;s->pending_low=low;s->pending_serial=++s->next_serial;
 s->pending_kind=pc==0x445b20u?1:pc==0x445bbeu?2:0;memcpy(s->pending_words,expected,sizeof expected);return 0;
}
int cod_pcm_registry_alloc_before_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t pc,uint32_t align,uint32_t n,uint32_t high,uint32_t low){return alloc_before(s,i,pc,align,n,high,low,NULL);}
int cod_pcm_registry_converted_before_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,const CodPcmConvertedV1*p,uint32_t align,uint32_t n,uint32_t high,uint32_t low){return alloc_before(s,i,0x445bbeu,align,n,high,low,p);}
int cod_pcm_registry_alloc_after_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t result,uint32_t high,uint32_t low){
 if(!idle(s,i)||!s->pending)return -1;
 if(high!=s->pending_high||low!=s->pending_low||result!=(uint64_t)s->hunk_guest+s->capacity-high||!extent(s,result,s->pending_requested))return poison(s);
 if(s->pending_kind){
  if(find(s,result)>=0)return poison(s);
  unsigned k;for(k=0;k<COD_PCM_CACHE_SLOTS_V1;k++)if(!s->caches[k].cache)break;if(k==COD_PCM_CACHE_SLOTS_V1)return poison(s);
  s->caches[k]=(CodPcmCacheEntryV1){.cache=result,.requested=s->pending_requested,.hunk_epoch=s->hunk_epoch,.serial=s->pending_serial,.kind=s->pending_kind};
  if(s->pending_kind==2){memcpy(s->caches[k].words,s->pending_words,sizeof s->pending_words);s->caches[k].words[1]=s->caches[k].words[8]=result+36;}
 }
 s->high=high;s->low=low;s->pending=0;return 0;
}
int cod_pcm_registry_publish_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t cache){
 if(!idle(s,i)||s->pending)return -1;int k=find(s,cache);if(k<0)return -1;CodPcmCacheEntryV1*e=s->caches+k;
 if(e->published||!extent(s,cache,e->requested))return -1;uint8_t*p=view(s,cache,36);uint32_t w[9];for(unsigned j=0;j<9;j++)w[j]=word(p+4*j);
 if((e->kind==1?!words_valid(w,cache,e->requested):e->kind==2?memcmp(w,e->words,sizeof w)!=0:1)||s->total_owned+w[2]>TOTAL_LIMIT)return -1;
 s->busy=1;uint8_t*copy=s->allocator.allocate(s->allocator.context,w[2]);s->busy=0;if(!copy)return -1;
 if(overlap(copy,w[2],s,sizeof *s)||overlap(copy,w[2],s->backing_memory,s->backing_bytes))return poison(s);
 for(unsigned j=0;j<COD_PCM_CACHE_SLOTS_V1;j++)if(s->caches[j].copy&&overlap(copy,w[2],s->caches[j].copy,s->caches[j].words[2]))return poison(s);
 memcpy(copy,p+36,w[2]);memcpy(e->words,w,sizeof w);e->copy=copy;e->published=1;s->total_owned+=w[2];return 0;
}
int cod_pcm_registry_alias_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t site,uint32_t alias,uint32_t cache){
 if(!idle(s,i)||s->pending||!alias||(alias&3u)||(site!=0x42c82du&&site!=0x42c84eu))return -1;
 int k=find(s,cache);if(k<0||!s->caches[k].published)return -1;unsigned at=COD_PCM_ALIAS_SLOTS_V1;
 for(unsigned j=0;j<COD_PCM_ALIAS_SLOTS_V1;j++){if(s->aliases[j].alias==alias)return -1;if(!s->aliases[j].alias&&at==COD_PCM_ALIAS_SLOTS_V1)at=j;}
 if(at==COD_PCM_ALIAS_SLOTS_V1)return -1;
 s->aliases[at]=(CodPcmAliasV1){alias,cache,s->caches[k].serial,s->hunk_epoch};return 0;
}
int cod_pcm_registry_acquire_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t alias,uint32_t cache,uint32_t high,uint32_t low,uint32_t address,uint32_t n,CodPcmLeaseV1*out){
 if(!out||!idle(s,i)||s->pending||low>s->capacity||high>s->capacity-low||s->next_lease==UINT64_MAX||overlap(out,sizeof *out,s,sizeof *s)||overlap(out,sizeof *out,s->backing_memory,s->backing_bytes))return -1;
 if(high!=s->high)return poison(s);
 int k=find(s,cache);if(k<0)return -1;CodPcmCacheEntryV1*e=s->caches+k;if(!e->published||address!=e->words[1]||n!=e->words[2])return -1;
 unsigned a;for(a=0;a<COD_PCM_ALIAS_SLOTS_V1;a++)if(s->aliases[a].alias==alias&&s->aliases[a].cache==cache&&s->aliases[a].cache_serial==e->serial&&s->aliases[a].hunk_epoch==s->hunk_epoch)break;
 if(a==COD_PCM_ALIAS_SLOTS_V1)return -1;uint8_t*p=view(s,cache,e->requested);if(!p)return -1;
 for(unsigned j=0;j<COD_PCM_CACHE_SLOTS_V1;j++)if(s->caches[j].copy&&overlap(out,sizeof *out,s->caches[j].copy,s->caches[j].words[2]))return -1;
 for(unsigned j=0;j<9;j++)if(word(p+4*j)!=e->words[j])return -1;
 if(memcmp(p+36,e->copy,n))return -1;
 s->borrowed=++s->next_lease;s->borrowed_index=(uint32_t)k;
 *out=(CodPcmLeaseV1){address,n,(e->words[4]==16?1u:0u)+(e->words[5]==2?2u:0u),e->words[3],e->copy,s->borrowed,e->serial,s->hunk_epoch};return 0;
}
int cod_pcm_registry_finish_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,const CodPcmLeaseV1*l,int copied){
 if(!live(s,i)||!l||!s->borrowed||l->lease!=s->borrowed||s->borrowed_index>=COD_PCM_CACHE_SLOTS_V1)return -1;
 CodPcmCacheEntryV1*e=s->caches+s->borrowed_index;
 if(l->cache_serial!=e->serial||l->hunk_epoch!=s->hunk_epoch||l->memory!=e->copy||l->address!=e->words[1]||l->bytes!=e->words[2]||
    l->format!=(e->words[4]==16?1u:0u)+(e->words[5]==2?2u:0u)||l->rate!=e->words[3])return -1;
 s->borrowed=0;return copied==1?0:poison(s);
}
int cod_pcm_registry_reset_before_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i){
 if(!idle(s,i)||s->pending||s->hunk_epoch==UINT64_MAX)return -1;
 s->reset_pending=1;clear(s);s->hunk_epoch++;return 0;
}
int cod_pcm_registry_reset_after_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t high,uint32_t low){
 if(!live(s,i)||!s->reset_pending)return -1;if(high||low)return poison(s);s->high=s->low=0;s->reset_pending=0;return 0;
}
int cod_pcm_registry_rewind_before_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t high,uint32_t low,uint32_t next){
 if(!idle(s,i)||s->pending||s->hunk_epoch==UINT64_MAX||low>s->capacity||high>s->capacity-low||next>high)return -1;
 if(high!=s->high)return poison(s);
 s->rewind_pending=1;s->rewind_high=next;s->rewind_low=low;s->hunk_epoch++;s->busy=1;
 uint64_t floor=(uint64_t)s->hunk_guest+s->capacity-next;
 for(unsigned k=0;k<COD_PCM_CACHE_SLOTS_V1;k++){
  CodPcmCacheEntryV1*e=s->caches+k;if(!e->cache)continue;
  if(e->cache<floor){if(e->copy){s->total_owned-=e->words[2];s->allocator.release(s->allocator.context,e->copy,e->words[2]);}memset(e,0,sizeof *e);}
  else e->hunk_epoch=s->hunk_epoch;
 }
 for(unsigned k=0;k<COD_PCM_ALIAS_SLOTS_V1;k++){
  CodPcmAliasV1*a=s->aliases+k;if(!a->alias)continue;int at=find(s,a->cache);
  if(at<0||s->caches[at].serial!=a->cache_serial)memset(a,0,sizeof *a);else a->hunk_epoch=s->hunk_epoch;
 }
 s->busy=0;return 0;
}
int cod_pcm_registry_rewind_after_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t high,uint32_t low){
 if(!live(s,i)||!s->rewind_pending||s->busy)return -1;
 if(high!=s->rewind_high||low!=s->rewind_low)return poison(s);
 s->high=high;s->low=low;s->rewind_pending=0;return 0;
}
int cod_pcm_registry_retire_aliases_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t table,uint32_t rows){
 if(!idle(s,i)||s->pending||!table||!rows||(uint64_t)table+(uint64_t)rows*68>UINT64_C(0x100000000))return -1;
 for(unsigned j=0;j<COD_PCM_ALIAS_SLOTS_V1;j++){uint32_t a=s->aliases[j].alias;if(a>=table&&(uint64_t)a<(uint64_t)table+(uint64_t)rows*68){if((a-table)%68)return poison(s);memset(s->aliases+j,0,sizeof s->aliases[j]);}}
 return 0;
}
int cod_pcm_registry_forget_alias_span_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i,uint32_t base,uint32_t bytes){
 if(!idle(s,i)||s->pending||!base||!bytes||(uint64_t)base+bytes>UINT64_C(0x100000000))return -1;
 for(unsigned j=0;j<COD_PCM_ALIAS_SLOTS_V1;j++){uint32_t a=s->aliases[j].alias;if(a>=base&&(uint64_t)a<(uint64_t)base+bytes)memset(s->aliases+j,0,sizeof s->aliases[j]);}
 return 0;
}
int cod_pcm_registry_retire_v1(CodPcmRegistryV1*s,const CodPcmIdentityV1*i){if(!live(s,i)||s->borrowed||s->pending||s->reset_pending||s->rewind_pending||s->busy)return -1;s->retained=1;clear(s);s->backing_serial=0;return 0;}
int cod_pcm_registry_dispose_v1(CodPcmRegistryV1*s){if(!s||!s->ready||s->borrowed||s->busy)return -1;s->retained=1;clear(s);s->ready=0;return 0;}
