#include "native_dispatch.h"
#define DISPATCH_MAGIC 0x50434453u
typedef struct {
 uint32_t magic,count,import_count,depth;
 void *host;
 const pc64_native_entry *entries;
 uint64_t import_base,import_end,remaining;
 pc64_native_import service;
 void *service_context;
} dispatch_state;
static _Thread_local dispatch_state *active_dispatch;
uint32_t pc64_dispatch_size(void){return (uint32_t)sizeof(dispatch_state);}
int32_t pc64_dispatch_init(void *storage,uint32_t bytes,void *host,
 const pc64_native_entry *entries,uint32_t count,uint64_t import_base,uint32_t import_count,
 pc64_native_import service,void *service_context) {
 if(!storage||((uintptr_t)storage&7)||bytes<sizeof(dispatch_state)||!host||!entries||!count)return PC64_PARAM;
 dispatch_state *s=(dispatch_state *)storage;if(s==active_dispatch)return PC64_DISPATCH_CONTRACT;
 s->magic=0;
 if(import_count&&((import_base&7)||!import_base||import_base>UINT64_MAX-(uint64_t)import_count*8))return PC64_PARAM;
 uint64_t address;
 for(uint32_t i=0;i<count;i++) {
  if(!entries[i].function||(i&&entries[i-1].preferred_address>=entries[i].preferred_address)||
     pc64_host_image_canonical(host,entries[i].preferred_address,&address)||address!=entries[i].preferred_address)return PC64_PARAM;
  uint64_t live;
  if(pc64_host_image_address(host,entries[i].preferred_address,&live))return PC64_PARAM;
  if(import_count&&((entries[i].preferred_address>=import_base&&entries[i].preferred_address<import_base+(uint64_t)import_count*8)||
      (live>=import_base&&live<import_base+(uint64_t)import_count*8)))return PC64_PARAM;
 }
 if(import_count) {
  if(pc64_host_image_canonical(host,import_base,&address)==0||
     pc64_host_image_canonical(host,import_base+(uint64_t)(import_count-1)*8,&address)==0)return PC64_PARAM;
 }
 s->host=host;s->entries=entries;s->count=count;s->import_count=import_count;
 s->import_base=import_base;s->import_end=import_base+(uint64_t)import_count*8;
 s->service=service;s->service_context=service_context;s->remaining=0;s->depth=0;s->magic=DISPATCH_MAGIC;return 0;
}
int32_t pc64_dispatch_bind(void *storage,uint64_t budget) {
 if(!storage||((uintptr_t)storage&7)||!budget)return PC64_PARAM;
 dispatch_state *s=(dispatch_state *)storage;
 if(s->magic!=DISPATCH_MAGIC||s->host!=pc64_active_host()||(active_dispatch&&active_dispatch->depth))return PC64_DISPATCH_CONTRACT;
 s->remaining=budget;s->depth=0;active_dispatch=s;return 0;
}
void pc64_dispatch_unbind_after_stop(void){if(active_dispatch)active_dispatch->depth=0;active_dispatch=0;}
uint64_t pc64_dispatch_entries_remaining(void){return active_dispatch?active_dispatch->remaining:0;}
uint32_t pc64_dispatch_depth(void){return active_dispatch?active_dispatch->depth:0;}
uint64_t pc64_canonical_indirect(uint64_t address) {
 uint64_t preferred;int32_t status=pc64_host_image_canonical(pc64_active_host(),address,&preferred);
 if(!status)return preferred;
 if(status==PC64_RANGE)return address;
 pc64_fail(status,address,0);
}
int es3_cs_depth_get(void) {
 if(!active_dispatch)pc64_fail(PC64_DISPATCH_CONTRACT,0,0);
 return (int)active_dispatch->depth;
}
void es3_cs_depth_set(int depth) {
 if(!active_dispatch||depth<0||(uint32_t)depth>active_dispatch->depth)pc64_fail(PC64_DISPATCH_CONTRACT,0,(uint32_t)depth);
 active_dispatch->depth=(uint32_t)depth;
}
static pc64_native_function find(dispatch_state *s,uint64_t canonical) {
 uint32_t lo=0,hi=s->count;
 while(lo<hi) {
  uint32_t mid=lo+(hi-lo)/2;uint64_t va=s->entries[mid].preferred_address;
  if(va==canonical)return s->entries[mid].function;
  if(va<canonical)lo=mid+1;else hi=mid;
 }
 return 0;
}
void dispatch(CPU *cpu,uint64_t target) {
 dispatch_state *s=active_dispatch;
 if(!s||!cpu||s->host!=pc64_active_host())pc64_fail(PC64_DISPATCH_CONTRACT,target,0);
 if(!s->remaining)pc64_fail(PC64_DISPATCH_BUDGET,target,0);
 if(s->depth>=512)pc64_fail(PC64_DISPATCH_DEPTH,target,s->depth);
 s->remaining--;
 if(s->import_count&&target>=s->import_base&&target<s->import_end) {
  uint64_t offset=target-s->import_base;
  if((offset&7)||!s->service)pc64_fail(PC64_UNSUPPORTED,target,0);
  uint64_t rsp=cpu->rsp;s->depth++;
  int32_t result=s->service(s->service_context,cpu,(uint32_t)(offset/8));
  if(result)pc64_fail(result,target,0);
  if(cpu->rsp!=rsp)pc64_fail(PC64_DISPATCH_CONTRACT,target,cpu->rsp);
  cpu->rip=pop64(cpu);s->depth--;return;
 }
 uint64_t canonical;int32_t status=pc64_host_image_canonical(s->host,target,&canonical);
 if(status)pc64_fail(status,target,0);
 pc64_native_function fn=find(s,canonical);if(!fn)pc64_fail(PC64_UNSUPPORTED,target,0);
 s->depth++;fn(cpu);s->depth--;
}
void dispatch_jmp(CPU *cpu,uint64_t target){dispatch(cpu,target);}
