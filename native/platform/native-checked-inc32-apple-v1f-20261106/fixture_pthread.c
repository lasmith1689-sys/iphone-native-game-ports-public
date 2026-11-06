#include "native_atomic_required.h"
#include "synthetic_profiles.h"
#include "synthetic_vectors.h"
#include "cookie_worker_tag.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define CHECK(v)do{atomic_fetch_add_explicit(&checks,1,memory_order_relaxed);if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(4);}}while(0)
enum{THREADS=4,ROUNDS=128};
static _Atomic unsigned checks;
typedef struct{uint32_t before;_Atomic uint32_t word;uint32_t after[2];}Record;
_Static_assert(sizeof(Record)==16&&offsetof(Record,word)==4&&ATOMIC_INT_LOCK_FREE==2,"valid DWORD and exact guards");
typedef struct{pthread_mutex_t lock;pthread_cond_t condition;unsigned arrived,generation;}Barrier;
static Barrier begin={PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,0,0},end={PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,0,0},inner={PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,0,0};
static void rendezvous(Barrier*b){CHECK(pthread_mutex_lock(&b->lock)==0);unsigned generation=b->generation;if(++b->arrived==THREADS){b->arrived=0;b->generation++;CHECK(pthread_cond_broadcast(&b->condition)==0);}else while(generation==b->generation)CHECK(pthread_cond_wait(&b->condition,&b->lock)==0);CHECK(pthread_mutex_unlock(&b->lock)==0);}
/* Explicit negative-host helper: every access is atomic; the two operations
 * are deliberately NOT one RMW. Rendezvous prevents nondeterministic excuses. */
uint32_t pc64_test_split_fetch_add(uint32_t*p,uint32_t value){uint32_t old=__atomic_load_n(p,__ATOMIC_SEQ_CST);rendezvous(&inner);__atomic_store_n(p,old+value,__ATOMIC_SEQ_CST);rendezvous(&inner);return old;}
uint32_t pc64_test_preread_v1(uint64_t address){uint32_t old=__atomic_load_n((uint32_t*)(uintptr_t)address,__ATOMIC_SEQ_CST);rendezvous(&inner);return old;}
typedef struct Worker Worker;
struct Worker{CPU cpu;void*context,*dispatcher;unsigned id,cases,faults,actorRejections,cohortCalls;uint32_t tag;jmp_buf landing;unsigned armed;int32_t status;uint32_t depth;uint64_t remaining,address,detail;};
typedef struct{void*host;unsigned char*image,*stack;_Alignas(8) Record record;_Atomic uint32_t outside,shared;const char*mode;const Pc64IncSiteV1*sites;uint32_t count;Worker*w[THREADS];uint64_t cohort[ROUNDS][THREADS][5];}Fixture;
static Fixture f;
static _Thread_local Worker*current;
static const pc64_native_entry entries[4]={
 {UINT64_C(0x10001000),synthetic_inc_0},{UINT64_C(0x10001010),synthetic_inc_1},{UINT64_C(0x10001020),synthetic_inc_2},{UINT64_C(0x10001030),synthetic_inc_3}};
static void on_fault(void*p,int32_t status,uint64_t address,uint64_t detail){Worker*s=p;s->status=status;s->address=address;s->detail=detail;s->depth=pc64_dispatch_depth();s->remaining=pc64_dispatch_entries_remaining();if(!s->armed){fprintf(stderr,"UNEXPECTED-FAULT %d\n",status);exit(5);}longjmp(s->landing,1);}
static void set_operand(CPU*c,unsigned index,uint64_t target){uint32_t kind=f.sites[index].operandKind;if(kind==PC64_INC_RCX)c->rcx=target;else if(kind==PC64_INC_RDX)c->rdx=target;else c->rax=target-(kind==PC64_INC_RAX_15C?UINT64_C(0x15c):0);}
static void input(Worker*s,unsigned index,uint64_t target){memset(&s->cpu,0x6b,sizeof s->cpu);s->cpu.rip=synthetic_markers[index];s->cpu.rsp=(uint64_t)(uintptr_t)(f.stack+32768);s->cpu.cf=1;s->cpu.zf=1;s->cpu.sf=1;s->cpu.of=1;s->cpu.pf=1;s->cpu.af=1;s->cpu.mxcsr=0x1f80;set_operand(&s->cpu,index,target);}
static void record_set(const unsigned char*b){memcpy(&f.record.before,b,4);uint32_t v;memcpy(&v,b+4,4);atomic_store_explicit(&f.record.word,v,memory_order_seq_cst);memcpy(f.record.after,b+8,8);}
static void record_get(unsigned char*b){memcpy(b,&f.record.before,4);uint32_t v=atomic_load_explicit(&f.record.word,memory_order_seq_cst);memcpy(b+4,&v,4);memcpy(b+8,f.record.after,8);}
static void registry(Worker*s,const pc64_native_entry*e,uint32_t count){CHECK(pc64_dispatch_init(s->dispatcher,pc64_dispatch_size(),f.host,e,count,0,0,NULL,NULL)==0);}
static void bind_frame(Worker*s){CHECK(pc64_dispatch_bind(s->dispatcher,16)==0);}
static void setup(Worker*s){
#if defined(__APPLE__)
 CHECK(pthread_main_np()==0);
#endif
 current=s;pc64_bind_host(f.host,on_fault,s);int old=-1;CHECK(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&old)==0);s->context=calloc(1,pc64_inc_binding_size_v1());s->dispatcher=calloc(1,pc64_dispatch_size());CHECK(s->context&&s->dispatcher);CHECK(pc64_inc_binding_init_v1(s->context,pc64_inc_binding_size_v1(),f.host,&s->cpu,f.sites,f.count)==0);CHECK(cookie_worker_tag_read_v1(&s->tag)==0&&s->tag);CHECK(pc64_inc_bind_v1(s->context)==0);registry(s,entries,4);}
static void stop(Worker*s){pc64_dispatch_unbind_after_stop();CHECK(pc64_inc_unbind_after_stop_v1()==0);CHECK(pc64_inc_unbind_after_stop_v1()==0);CHECK(pc64_inc_bind_v1(s->context)==PC64_INC_STATE);pc64_bind_host(NULL,NULL,NULL);}
static void synthetic_cases(Worker*s){
 for(unsigned index=0;index<4;index++)for(unsigned k=0;k<256;k++){
  const SyntheticIncVector*v=&synthetic_vectors[k];uint64_t target=(uint64_t)(uintptr_t)&f.record.word;input(s,index,target);
  for(unsigned z=0;z<448;z++)((unsigned char*)&s->cpu)[z]^=(unsigned char)(k*37+z*13);
  s->cpu.rip=synthetic_markers[index];s->cpu.rsp=(uint64_t)(uintptr_t)(f.stack+32768);set_operand(&s->cpu,index,target);s->cpu.cf=v->cf;s->cpu.zf=(k>>1)&1;s->cpu.sf=(k>>2)&1;s->cpu.of=(k>>3)&1;s->cpu.pf=(k>>4)&1;s->cpu.af=(k>>5)&1;s->cpu.mxcsr=0x1f80;
  CPU expected;memcpy(&expected,&s->cpu,448);expected.zf=v->zf;expected.sf=v->sf;expected.of=v->of;expected.pf=v->pf;expected.af=v->af;
  unsigned char before[16],wanted[16];memset(before,(int)(0x5d^k^index),16);memcpy(before+4,&v->old,4);memcpy(wanted,before,16);memcpy(wanted+4,&v->result,4);record_set(before);bind_frame(s);s->armed=1;s->status=0;if(setjmp(s->landing)==0)dispatch(&s->cpu,synthetic_source_vas[index]);s->armed=0;CHECK(s->status==0);
  CHECK(memcmp(&s->cpu,&expected,448)==0);unsigned char actual[16];record_get(actual);CHECK(memcmp(actual,wanted,16)==0);CHECK(pc64_dispatch_depth()==0&&pc64_dispatch_entries_remaining()==15);pc64_dispatch_unbind_after_stop();s->cases++;
 }
}
static void data_faults(Worker*s){
 for(unsigned index=0;index<4;index++){
  uint64_t target=(uint64_t)(uintptr_t)&f.record.word;int32_t expectedStatus=PC64_PERMISSION;unsigned char before[16];memset(before,0x5d,16);record_set(before);
  if(!strcmp(f.mode,"unaligned")){target++;expectedStatus=PC64_ALIGNMENT;}
  if(!strcmp(f.mode,"range")){target=(uint64_t)(uintptr_t)&f.outside;expectedStatus=PC64_RANGE;}
  if(!strcmp(f.mode,"alias")){target=(uint64_t)(uintptr_t)&s->cpu.cf;expectedStatus=PC64_INC_ALIAS;}
  input(s,index,target);CPU expected;memcpy(&expected,&s->cpu,448);expected.rip=f.sites[index].sourceVA;bind_frame(s);s->armed=1;s->status=0;
  if(setjmp(s->landing)==0){dispatch(&s->cpu,f.sites[index].sourceVA);CHECK(0);}s->armed=0;
  CHECK(s->status==expectedStatus&&s->address==target&&s->detail==4);CHECK(s->depth==1&&s->remaining==15);CHECK(memcmp(&s->cpu,&expected,448)==0);unsigned char actual[16];record_get(actual);CHECK(memcmp(actual,before,16)==0);pc64_dispatch_unbind_after_stop();s->faults++;
 }
}
static void source_faults(Worker*s){
 for(unsigned index=0;index<4;index++){
  unsigned char before[16];memset(before,0x4e,16);record_set(before);input(s,index,(uint64_t)(uintptr_t)&f.record.word);unsigned char*opcode=f.image+(f.sites[index].sourceVA-UINT64_C(0x10000000));int changed=!strcmp(f.mode,"opcode");if(changed)*opcode^=1;else s->cpu.rip^=1;CPU expected;memcpy(&expected,&s->cpu,448);expected.rip=f.sites[index].sourceVA;bind_frame(s);s->armed=1;
  if(setjmp(s->landing)==0){dispatch(&s->cpu,f.sites[index].sourceVA);CHECK(0);}s->armed=0;
  CHECK(s->status==(changed?PC64_INC_IMAGE:PC64_INC_SITE));CHECK(s->depth==1&&s->remaining==15);CHECK(memcmp(&s->cpu,&expected,448)==0);unsigned char actual[16];record_get(actual);CHECK(memcmp(actual,before,16)==0);pc64_dispatch_unbind_after_stop();if(changed)*opcode^=1;s->faults++;
 }
}
static void invalid_cpu_probe(CPU*c){(void)c;pc64_required_inc32_v1((CPU*)(uintptr_t)1,0,f.sites[0].sourceVA);}
static void rejected_unbind(CPU*c){(void)c;CHECK(pc64_inc_unbind_after_stop_v1()==PC64_INC_STATE);current->actorRejections++;}
static void actor_fault(Worker*s,int32_t wanted,int frame){
 CPU before;memcpy(&before,&s->cpu,448);if(frame){static pc64_native_entry probe={UINT64_C(0x10001000),invalid_cpu_probe};registry(s,&probe,1);bind_frame(s);}s->armed=1;
 if(setjmp(s->landing)==0){if(frame)dispatch(&s->cpu,f.sites[0].sourceVA);else pc64_required_inc32_v1(&s->cpu,0,f.sites[0].sourceVA);CHECK(0);}s->armed=0;
 CHECK(s->status==wanted);CHECK(memcmp(&s->cpu,&before,448)==0);CHECK(s->depth==(uint32_t)frame&&s->remaining==(uint64_t)(frame?15:0));pc64_dispatch_unbind_after_stop();s->actorRejections++;
}
static void*foreign(void*p){Worker*owner=p;Worker*s=calloc(1,sizeof* s);CHECK(s);setup(s);CHECK(s->tag!=owner->tag);CHECK(pc64_inc_bind_v1(owner->context)==PC64_INC_WORKER);s->actorRejections++;input(s,0,0);actor_fault(s,PC64_INC_CPU,1);stop(s);return s;}
static void admission(Worker*s){input(s,0,0);actor_fault(s,PC64_INC_STATE,0);static pc64_native_entry e={UINT64_C(0x10001000),rejected_unbind};registry(s,&e,1);bind_frame(s);dispatch(&s->cpu,f.sites[0].sourceVA);CHECK(pc64_dispatch_depth()==0);pc64_dispatch_unbind_after_stop();actor_fault(s,PC64_INC_CPU,1);
 CPU before;memcpy(&before,&s->cpu,448);pthread_t t;CHECK(pthread_create(&t,NULL,foreign,s)==0);void*out=NULL;CHECK(pthread_join(t,&out)==0);CHECK(out&&((Worker*)out)->actorRejections==2);CHECK(memcmp(&before,&s->cpu,448)==0);s->actorRejections+=2;
 CHECK(pc64_inc_unbind_after_stop_v1()==0);actor_fault(s,PC64_INC_CONTEXT,1);
}
static void*owner_main(void*p){Worker*s=p;setup(s);if(!strcmp(f.mode,"normal"))synthetic_cases(s);else if(!strcmp(f.mode,"readonly")||!strcmp(f.mode,"unaligned")||!strcmp(f.mode,"range")||!strcmp(f.mode,"alias"))data_faults(s);else if(!strcmp(f.mode,"marker")||!strcmp(f.mode,"opcode"))source_faults(s);else admission(s);stop(s);return NULL;}
static void five_flags(CPU*c,uint64_t*out){out[0]=c->zf;out[1]=c->sf;out[2]=c->of;out[3]=c->pf;out[4]=c->af;}
static int same408(const CPU*a,const CPU*b){CPU x,y;memcpy(&x,a,448);memcpy(&y,b,448);x.zf=y.zf=0;x.sf=y.sf=0;x.of=y.of=0;x.pf=y.pf=0;x.af=y.af=0;return memcmp(&x,&y,448)==0;}
static void*cohort_worker(void*p){Worker*s=p;setup(s);for(unsigned round=0;round<ROUNDS;round++){
 input(s,1,(uint64_t)(uintptr_t)&f.shared);s->cpu.cf=s->id&1;CPU before;memcpy(&before,&s->cpu,448);rendezvous(&begin);bind_frame(s);dispatch(&s->cpu,f.sites[1].sourceVA);CHECK(pc64_dispatch_depth()==0&&pc64_dispatch_entries_remaining()==15);CHECK(same408(&s->cpu,&before));five_flags(&s->cpu,f.cohort[round][s->id]);pc64_dispatch_unbind_after_stop();s->cohortCalls++;rendezvous(&end);
 }stop(s);return NULL;}
static int cohort_flags_match(unsigned round){unsigned used=0;for(unsigned t=0;t<THREADS;t++){int found=0;for(unsigned i=0;i<THREADS;i++)if(!(used&(1u<<i))&&!memcmp(f.cohort[round][t],cohort_expected[round*THREADS+i],40)){used|=1u<<i;found=1;break;}if(!found)return 0;}return used==15;}
int main(int argc,char**argv){if(argc!=2)return 2;f.mode=argv[1];
#if !defined(__APPLE__)
 return 2; /* Only genuine Darwin headers/runtime are admitted for this fixture. */
#endif
if(strcmp(f.mode,"normal")&&strcmp(f.mode,"readonly")&&strcmp(f.mode,"unaligned")&&strcmp(f.mode,"range")&&strcmp(f.mode,"alias")&&strcmp(f.mode,"marker")&&strcmp(f.mode,"opcode")&&strcmp(f.mode,"admission")&&strcmp(f.mode,"cohort"))return 2;
 f.sites=synthetic_inc_sites_v1(&f.count);CHECK(f.count==4);f.host=calloc(1,pc64_host_size());f.image=calloc(1,8192);f.stack=calloc(1,65536);CHECK(f.host&&f.image&&f.stack);CHECK(pc64_host_init(f.host,pc64_host_size())==0);CHECK(pc64_host_set_image(f.host,UINT64_C(0x10000000),f.image,8192)==0);
 for(unsigned i=0;i<4;i++){unsigned char*p=f.image+(f.sites[i].sourceVA-UINT64_C(0x10000000));memcpy(p,f.sites[i].originalBytes,f.sites[i].byteCount);CHECK(pc64_host_add_region(f.host,p,f.sites[i].byteCount,PC64_READ)==0);}
 CHECK(pc64_host_add_region(f.host,&f.record,sizeof f.record,!strcmp(f.mode,"readonly")?PC64_READ:PC64_READ|PC64_WRITE)==0);CHECK(pc64_host_add_region(f.host,&f.shared,4,PC64_READ|PC64_WRITE)==0);CHECK(pc64_host_add_region(f.host,f.stack,65536,PC64_READ|PC64_WRITE)==0);/* Main role is refused before a tag is warmed or CPU state changes. */
 #if defined(__APPLE__)
 Worker*mainState=calloc(1,sizeof(Worker));CHECK(mainState);mainState->context=calloc(1,pc64_inc_binding_size_v1());CHECK(mainState->context);memset(&mainState->cpu,0x5a,448);CPU mainBefore;memcpy(&mainBefore,&mainState->cpu,448);pc64_bind_host(f.host,on_fault,mainState);CHECK(pthread_main_np()!=0);uint32_t mainTag=0xa5a5a5a5;CHECK(cookie_worker_tag_read_v1(&mainTag)==COOKIE_TAG_UNBOUND_V1&&mainTag==0xa5a5a5a5);CHECK(pc64_inc_binding_init_v1(mainState->context,pc64_inc_binding_size_v1(),f.host,&mainState->cpu,f.sites,f.count)==PC64_INC_WORKER);CHECK(memcmp(&mainState->cpu,&mainBefore,448)==0);CHECK(cookie_worker_tag_read_v1(&mainTag)==COOKIE_TAG_UNBOUND_V1&&mainTag==0xa5a5a5a5);pc64_bind_host(NULL,NULL,NULL);
 #endif
 atomic_init(&f.record.word,0);atomic_init(&f.outside,0);atomic_init(&f.shared,0);unsigned workers=!strcmp(f.mode,"cohort")?THREADS:1;pthread_t threads[THREADS];
 for(unsigned i=0;i<workers;i++){f.w[i]=calloc(1,sizeof(Worker));CHECK(f.w[i]);f.w[i]->id=i;CHECK(pthread_create(&threads[i],NULL,workers==THREADS?cohort_worker:owner_main,f.w[i])==0);}for(unsigned i=0;i<workers;i++)CHECK(pthread_join(threads[i],NULL)==0);
 unsigned cases=0,faults=0,actors=0,cohort=0;for(unsigned i=0;i<workers;i++){cases+=f.w[i]->cases;faults+=f.w[i]->faults;actors+=f.w[i]->actorRejections;cohort+=f.w[i]->cohortCalls;}
 if(workers==THREADS){uint32_t finalShared=atomic_load_explicit(&f.shared,memory_order_seq_cst);CHECK(finalShared==512);for(unsigned r=0;r<ROUNDS;r++)CHECK(cohort_flags_match(r));}
 printf("PASS checked-inc32 mode=%s syntheticCases=%u faults=%u actorRejections=%u cohortCalls=%u workers=%u mainRefused=1 actualPthreads=1 syntheticInputs=1 sourceReady=0\n",f.mode,cases,faults,actors,cohort,workers);return 0;}
