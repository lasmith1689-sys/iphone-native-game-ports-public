#include "native_atomic_required.h"
#include "cookie_worker_tag.h"
#include "ngp_fls_v1.h"
#include "ngp_worker_tag_v1.h"
#include "synthetic_profiles.h"
#include "synthetic_vectors.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static _Atomic unsigned checks;
#define CHECK(v) do {atomic_fetch_add_explicit(&checks,1,memory_order_relaxed);if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(4);}}while(0)
#define REQUIRE(v) do {if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(4);}}while(0)
typedef struct {uint32_t before;_Atomic uint32_t word;uint32_t after[2];} Record;
_Static_assert(sizeof(Record)==16&&offsetof(Record,word)==4&&ATOMIC_INT_LOCK_FREE==2,"actual guarded atomic DWORD");
typedef struct {
 void *host,*service,*inc,*dispatcher;unsigned char *image,*stack;
 CPU cpu;Record record;const Pc64IncSiteV1 *sites;uint32_t site_count;
 uint32_t token,owner_tag,aux_tag,values[65];void *expected;
 const char *mode;unsigned action,cases,faults,boxes,late_event;
 _Atomic unsigned callbacks,foreign_ops;
 jmp_buf landing;unsigned armed;int32_t fault_status;uint32_t fault_depth;uint64_t fault_remaining;
 pthread_t main_thread;
} Fixture;
static Fixture f;
static pthread_mutex_t coordination=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static unsigned aux_ready,foreign_go,foreign_done,aux_stop;
enum {ACTION_ALLOCATE=1,ACTION_CLOSED=2};

static NgpFlsReportV1 report(void){NgpFlsReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int main_thread(void){
#if defined(__APPLE__)
 return pthread_main_np()!=0;
#else
 return pthread_equal(pthread_self(),f.main_thread)!=0;
#endif
}
static void on_fault(void *context,int32_t status,uint64_t address,uint64_t detail){
 (void)address;(void)detail;Fixture *p=context;
 p->fault_status=status;p->fault_depth=pc64_dispatch_depth();p->fault_remaining=pc64_dispatch_entries_remaining();
 if(!p->armed){fprintf(stderr,"UNEXPECTED-FAULT %d\n",status);exit(5);}
 longjmp(p->landing,1);
}
static void authority(void){
 CHECK(ngp_fls_environment_host_bytes_v1()==pc64_host_size());
 CHECK(ngp_fls_environment_active_v1()==pc64_active_host());
 CHECK(pc64_active_host()==f.host);
 CHECK(ngp_fls_environment_depth_v1()==pc64_dispatch_depth());
 uint32_t cookie=0,alias=0;
 CHECK(cookie_worker_tag_read_v1(&cookie)==0&&cookie==f.owner_tag);
 CHECK(ngp_worker_tag_read_v1(&alias)==0&&alias==cookie);
}
static int32_t own_callback(void *context,void *value){
 Fixture *p=context;atomic_fetch_add_explicit(&p->callbacks,1,memory_order_relaxed);
 if(value!=p->expected||pc64_dispatch_depth()!=0||ngp_fls_environment_depth_v1()!=0)return 701;
 NgpFlsReportV1 r=report();
 if(ngp_fls_retire_v1(p->service,p->token,&r)!=NGP_FLS_STATE||r.phase!=NGP_FLS_RETIRING||!r.value_clear_ack||r.callback_ack||r.key_delete_ack)return 702;
 return !strcmp(p->mode,"callback-fail")?77:0;
}
static void fls_entry(CPU *cpu){
 CHECK(cpu==&f.cpu&&pc64_dispatch_depth()==1);authority();
 NgpFlsReportV1 r=report();
 if(f.action==ACTION_CLOSED){
  unsigned callbacks=atomic_load_explicit(&f.callbacks,memory_order_relaxed);
  CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==0&&r.phase==NGP_FLS_CLOSED);
  CHECK(r.value_clear_ack&&r.callback_ack&&r.key_delete_ack&&atomic_load_explicit(&f.callbacks,memory_order_relaxed)==callbacks);
  return;
 }
 CHECK(f.action==ACTION_ALLOCATE);f.token=UINT32_MAX;
 CHECK(ngp_fls_allocate_v1(f.service,&f.token,&r)==0&&f.token==1&&r.key_created&&r.phase==NGP_FLS_LIVE);
 void *out=(void*)(uintptr_t)0x1358;r=report();
 CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==0&&out==NULL);
 if(strcmp(f.mode,"capacity")){
  f.expected=&f.values[0];r=report();CHECK(ngp_fls_set_v1(f.service,f.token,f.expected,&r)==0&&r.boxes_used==1);
  f.expected=&f.values[1];r=report();CHECK(ngp_fls_set_v1(f.service,f.token,f.expected,&r)==0&&r.boxes_used==2);
  r=report();CHECK(ngp_fls_set_v1(f.service,f.token,NULL,&r)==0&&r.boxes_used==2);
  out=(void*)(uintptr_t)0x2468;r=report();CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==0&&out==NULL);
  f.expected=&f.values[2];r=report();CHECK(ngp_fls_set_v1(f.service,f.token,f.expected,&r)==0&&r.boxes_used==3);f.boxes=3;
 }
 uint32_t sentinel=0xa5a5a5a5;
 CHECK(ngp_worker_tag_warm_v1(&sentinel)==NGP_FLS_ENV_FRAME&&sentinel==0xa5a5a5a5);
 CHECK(cookie_worker_tag_warm_v1(&sentinel)==PC64_COUNTER_FRAME_V1&&sentinel==0xa5a5a5a5);
 r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_STATE);
 CHECK(r.phase==NGP_FLS_LIVE&&r.key_created&&!r.value_clear_ack&&!r.callback_attempted&&!r.callback_ack&&!r.key_delete_ack);
 CHECK(!atomic_load_explicit(&f.callbacks,memory_order_relaxed));
}
static void before_inc(CPU *cpu){
 CHECK(cpu==&f.cpu&&pc64_dispatch_depth()==1);authority();
 NgpFlsReportV1 r=report();void *out=NULL;
 CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==0&&out==f.expected);
}
static void inc0(CPU*c){before_inc(c);synthetic_inc_0(c);}
static void inc1(CPU*c){before_inc(c);synthetic_inc_1(c);}
static void inc2(CPU*c){before_inc(c);synthetic_inc_2(c);}
static void inc3(CPU*c){before_inc(c);synthetic_inc_3(c);}
static const pc64_native_entry entries[5]={
 {UINT64_C(0x10001000),inc0},{UINT64_C(0x10001010),inc1},
 {UINT64_C(0x10001020),inc2},{UINT64_C(0x10001030),inc3},
 {UINT64_C(0x10001100),fls_entry}
};
static void begin_frame(void){CHECK(pc64_dispatch_depth()==0);CHECK(pc64_dispatch_bind(f.dispatcher,32)==0);}
static void end_returned_frame(void){
 CHECK(pc64_dispatch_depth()==0&&pc64_dispatch_entries_remaining()==31);
 pc64_dispatch_unbind_after_stop();
}
static void call_fls(unsigned action){
 CPU before;memcpy(&before,&f.cpu,sizeof before);f.action=action;
 begin_frame();dispatch(&f.cpu,UINT64_C(0x10001100));end_returned_frame();
 CHECK(memcmp(&f.cpu,&before,448)==0);
}
static void operand(unsigned index,uint64_t target){
 uint32_t kind=f.sites[index].operandKind;
 if(kind==PC64_INC_RCX)f.cpu.rcx=target;
 else if(kind==PC64_INC_RDX)f.cpu.rdx=target;
 else f.cpu.rax=target-(kind==PC64_INC_RAX_15C?UINT64_C(0x15c):0);
}
static void input(unsigned index,uint64_t target){
 memset(&f.cpu,0x6b,448);f.cpu.rip=synthetic_markers[index];
 f.cpu.rsp=(uint64_t)(uintptr_t)(f.stack+32768);f.cpu.mxcsr=0x1f80;operand(index,target);
}
static void put_record(const unsigned char*b){
 memcpy(&f.record.before,b,4);uint32_t value;memcpy(&value,b+4,4);
 atomic_store_explicit(&f.record.word,value,memory_order_seq_cst);memcpy(f.record.after,b+8,8);
}
static void get_record(unsigned char*b){
 memcpy(b,&f.record.before,4);uint32_t value=atomic_load_explicit(&f.record.word,memory_order_seq_cst);
 memcpy(b+4,&value,4);memcpy(b+8,f.record.after,8);
}
static void vector_cases(void){
 for(unsigned index=0;index<4;index++)for(unsigned k=0;k<256;k++){
  const SyntheticIncVector *v=&synthetic_vectors[k];uint64_t target=(uint64_t)(uintptr_t)&f.record.word;
  input(index,target);for(unsigned z=0;z<448;z++)((unsigned char*)&f.cpu)[z]^=(unsigned char)(k*37+z*13);
  f.cpu.rip=synthetic_markers[index];f.cpu.rsp=(uint64_t)(uintptr_t)(f.stack+32768);operand(index,target);
  f.cpu.cf=v->cf;f.cpu.zf=(k>>1)&1;f.cpu.sf=(k>>2)&1;f.cpu.of=(k>>3)&1;f.cpu.pf=(k>>4)&1;f.cpu.af=(k>>5)&1;f.cpu.mxcsr=0x1f80;
  CPU expected;memcpy(&expected,&f.cpu,448);expected.zf=v->zf;expected.sf=v->sf;expected.of=v->of;expected.pf=v->pf;expected.af=v->af;
  unsigned char before[16],wanted[16];memset(before,(int)(0x5d^k^index),16);memcpy(before+4,&v->old,4);memcpy(wanted,before,16);memcpy(wanted+4,&v->result,4);put_record(before);
  begin_frame();f.armed=1;f.fault_status=0;
  if(setjmp(f.landing)==0)dispatch(&f.cpu,synthetic_source_vas[index]);
  f.armed=0;CHECK(f.fault_status==0);end_returned_frame();
  CHECK(memcmp(&f.cpu,&expected,448)==0);unsigned char actual[16];get_record(actual);CHECK(memcmp(actual,wanted,16)==0);f.cases++;
 }
}
static void fault_cases(void){
 for(unsigned index=0;index<4;index++){
  unsigned char before[16];memset(before,0x5d,16);put_record(before);
  input(index,(uint64_t)(uintptr_t)&f.record.word);CPU expected;memcpy(&expected,&f.cpu,448);expected.rip=f.sites[index].sourceVA;
  begin_frame();f.armed=1;f.fault_status=0;
  if(setjmp(f.landing)==0){dispatch(&f.cpu,synthetic_source_vas[index]);CHECK(0);}
  f.armed=0;CHECK(f.fault_status==PC64_PERMISSION&&f.fault_depth==1&&f.fault_remaining==31);
  CHECK(memcmp(&f.cpu,&expected,448)==0);unsigned char actual[16];get_record(actual);CHECK(memcmp(actual,before,16)==0);
  /* This is the actual outer fault landing pad. The invocation is discarded;
   * no guest state resumes. The next loop creates a fresh own input case. */
  pc64_dispatch_unbind_after_stop();CHECK(pc64_dispatch_depth()==0);f.faults++;
 }
}
static void *auxiliary(void *unused){
 (void)unused;CHECK(!main_thread());uint32_t tag=0;CHECK(cookie_worker_tag_warm_v1(&tag)==0&&tag);
 REQUIRE(pthread_mutex_lock(&coordination)==0);f.aux_tag=tag;aux_ready=1;REQUIRE(pthread_cond_broadcast(&condition)==0);
 while(!foreign_go&&!aux_stop)REQUIRE(pthread_cond_wait(&condition,&coordination)==0);
 unsigned run=foreign_go;REQUIRE(pthread_mutex_unlock(&coordination)==0);
 if(run){
  pc64_bind_host(f.host,NULL,NULL);uint32_t alias=0;CHECK(ngp_worker_tag_read_v1(&alias)==0&&alias==tag&&alias!=f.owner_tag);
  for(unsigned i=0;i<32;i++){
   NgpFlsReportV1 r=report();void *out=(void*)(uintptr_t)0x1358;
   CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==NGP_FLS_OWNER&&out==(void*)(uintptr_t)0x1358&&!r.worker_observation&&!r.token&&!r.boxes_used&&!r.key_created);
   atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_fls_set_v1(f.service,f.token,&f.values[5],&r)==NGP_FLS_OWNER&&!r.worker_observation);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_OWNER&&!r.worker_observation);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
  }
  CHECK(pc64_inc_bind_v1(f.inc)==PC64_INC_WORKER);pc64_bind_host(NULL,NULL,NULL);
  REQUIRE(pthread_mutex_lock(&coordination)==0);foreign_done=1;REQUIRE(pthread_cond_broadcast(&condition)==0);REQUIRE(pthread_mutex_unlock(&coordination)==0);
 }
 REQUIRE(pthread_mutex_lock(&coordination)==0);
 while(!aux_stop)REQUIRE(pthread_cond_wait(&condition,&coordination)==0);
 REQUIRE(pthread_mutex_unlock(&coordination)==0);return NULL;
}
static void terminal(unsigned callbacks,unsigned acks,int32_t native_error){
 NgpFlsReportV1 r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_STATE&&r.phase==NGP_FLS_FAILED&&r.native_error==native_error);
 CHECK(r.value_clear_ack==(acks&1)&&r.callback_ack==((acks>>1)&1)&&r.key_delete_ack==((acks>>2)&1));
 CHECK(atomic_load_explicit(&f.callbacks,memory_order_relaxed)==callbacks);
 r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_STATE&&atomic_load_explicit(&f.callbacks,memory_order_relaxed)==callbacks);
 void *out=(void*)(uintptr_t)0x2468;r=report();CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==NGP_FLS_STATE&&out==(void*)(uintptr_t)0x2468);
}
static void close_own(void){
 NgpFlsReportV1 r=report();CHECK(pc64_dispatch_depth()==0);
 CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==0&&r.phase==NGP_FLS_CLOSED&&r.value_clear_ack&&r.callback_ack&&r.key_delete_ack);
 CHECK(atomic_load_explicit(&f.callbacks,memory_order_relaxed)==(unsigned)(f.expected!=NULL));
 call_fls(ACTION_CLOSED);void *out=(void*)(uintptr_t)0x3578;r=report();
 CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==NGP_FLS_STATE&&out==(void*)(uintptr_t)0x3578);
}
static void *owner(void *unused){
 (void)unused;CHECK(!main_thread());pc64_bind_host(f.host,on_fault,&f);CHECK(cookie_worker_tag_warm_v1(&f.owner_tag)==0&&f.owner_tag!=f.aux_tag);
 uint32_t aliasTag=0;CHECK(ngp_worker_tag_warm_v1(&aliasTag)==0);CHECK(aliasTag==f.owner_tag);
 authority();NgpFlsCallbackV1 callback={own_callback,&f};NgpFlsReportV1 r=report();
 CHECK(ngp_fls_init_v1(f.service,ngp_fls_size_v1(),f.host,&callback,&r)==0&&r.phase==NGP_FLS_INIT);
 int prior=-1;CHECK(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&prior)==0&&prior==PTHREAD_CANCEL_DISABLE);
 CHECK(pc64_inc_binding_init_v1(f.inc,pc64_inc_binding_size_v1(),f.host,&f.cpu,f.sites,f.site_count)==0);CHECK(pc64_inc_bind_v1(f.inc)==0);
 CHECK(pc64_dispatch_init(f.dispatcher,pc64_dispatch_size(),f.host,entries,5,0,0,NULL,NULL)==0);
#ifdef NGP_FLS_TESTING
 if(!strcmp(f.mode,"create-fail")){
  CHECK(ngp_fls_test_fault_v1(f.service,1)==0);f.token=UINT32_MAX;r=report();
  CHECK(ngp_fls_allocate_v1(f.service,&f.token,&r)==NGP_FLS_RETAINED&&f.token==UINT32_MAX&&r.key_created&&r.phase==NGP_FLS_FAILED);terminal(0,0,-9001);goto done;
 }
#endif
 call_fls(ACTION_ALLOCATE);
 if(!strcmp(f.mode,"capacity")){
  for(unsigned i=0;i<64;i++){r=report();CHECK(ngp_fls_set_v1(f.service,f.token,&f.values[i],&r)==0&&r.boxes_used==i+1);}
  f.boxes=64;r=report();CHECK(ngp_fls_set_v1(f.service,f.token,&f.values[64],&r)==NGP_FLS_LIMIT&&r.phase==NGP_FLS_LIVE&&r.boxes_used==64);
  void*out=NULL;r=report();CHECK(ngp_fls_get_v1(f.service,f.token,&out,&r)==0&&out==&f.values[63]);
  r=report();CHECK(ngp_fls_set_v1(f.service,f.token,NULL,&r)==0&&r.boxes_used==64);f.expected=NULL;close_own();goto done;
 }
 if(!strcmp(f.mode,"owner-exit"))goto done;
 if(!strcmp(f.mode,"callback-fail")){r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_CALLBACK&&r.callback_attempted);terminal(1,1,77);goto done;}
#ifdef NGP_FLS_TESTING
 if(!strcmp(f.mode,"set-fail")){CHECK(ngp_fls_test_fault_v1(f.service,2)==0);r=report();CHECK(ngp_fls_set_v1(f.service,f.token,&f.values[3],&r)==NGP_FLS_RETAINED&&r.boxes_used==4);f.boxes=4;terminal(0,0,-9001);goto done;}
 if(!strcmp(f.mode,"clear-fail")||!strcmp(f.mode,"delete-fail")){
  unsigned deleted=!strcmp(f.mode,"delete-fail");CHECK(ngp_fls_test_fault_v1(f.service,deleted?4:3)==0);r=report();CHECK(ngp_fls_retire_v1(f.service,f.token,&r)==NGP_FLS_RETAINED);terminal(deleted?1:0,deleted?3:0,-9001);goto done;
 }
#endif
 if(!strcmp(f.mode,"normal")){
  REQUIRE(pthread_mutex_lock(&coordination)==0);foreign_go=1;REQUIRE(pthread_cond_broadcast(&condition)==0);REQUIRE(pthread_mutex_unlock(&coordination)==0);
  vector_cases();REQUIRE(pthread_mutex_lock(&coordination)==0);while(!foreign_done)REQUIRE(pthread_cond_wait(&condition,&coordination)==0);REQUIRE(pthread_mutex_unlock(&coordination)==0);
 }else if(!strcmp(f.mode,"atomic-fault"))fault_cases();
 close_own();
#ifdef NGP_FLS_TESTING
 if(!strcmp(f.mode,"late-exit")){CHECK(ngp_fls_test_late_exit_v1(f.service)==0);f.late_event=1;r=report();CHECK(ngp_fls_observe_v1(f.service,&r)==0&&r.phase==NGP_FLS_CLOSED&&r.owner_exit_observed);}
#endif
done:
 CHECK(pc64_dispatch_depth()==0);CHECK(pc64_inc_unbind_after_stop_v1()==0);pc64_bind_host(NULL,NULL,NULL);return NULL;
}
int main(int argc,char**argv){
#if !defined(__APPLE__)
 (void)argc;(void)argv;return 2; /* A Windows syntax object is not a Mac runtime result. */
#endif
 if(argc!=2)return 2;f.main_thread=pthread_self();f.mode=argv[1];CHECK(main_thread());
 int known=!strcmp(f.mode,"normal")||!strcmp(f.mode,"capacity")||!strcmp(f.mode,"owner-exit")||!strcmp(f.mode,"callback-fail")||!strcmp(f.mode,"atomic-fault");
#ifdef NGP_FLS_TESTING
 known|=!strcmp(f.mode,"create-fail")||!strcmp(f.mode,"set-fail")||!strcmp(f.mode,"clear-fail")||!strcmp(f.mode,"delete-fail")||!strcmp(f.mode,"late-exit");
#endif
 CHECK(known&&ngp_fls_prepare_process_v1()==0&&ngp_fls_prepare_process_v1()==NGP_FLS_STATE);
 f.sites=synthetic_inc_sites_v1(&f.site_count);CHECK(f.site_count==4);
 f.host=calloc(1,pc64_host_size());f.service=calloc(1,ngp_fls_size_v1());f.inc=calloc(1,pc64_inc_binding_size_v1());f.dispatcher=calloc(1,pc64_dispatch_size());f.image=calloc(1,8192);f.stack=calloc(1,65536);
 CHECK(f.host&&f.service&&f.inc&&f.dispatcher&&f.image&&f.stack);CHECK((uintptr_t)f.service%ngp_fls_alignment_v1()==0&&(uintptr_t)f.inc%pc64_inc_binding_alignment_v1()==0);
 CHECK(pc64_host_init(f.host,pc64_host_size())==0&&pc64_host_set_image(f.host,UINT64_C(0x10000000),f.image,8192)==0);
 for(unsigned i=0;i<4;i++){unsigned char*p=f.image+(f.sites[i].sourceVA-UINT64_C(0x10000000));memcpy(p,f.sites[i].originalBytes,f.sites[i].byteCount);CHECK(pc64_host_add_region(f.host,p,f.sites[i].byteCount,PC64_READ)==0);}
 CHECK(pc64_host_add_region(f.host,&f.record,sizeof f.record,!strcmp(f.mode,"atomic-fault")?PC64_READ:PC64_READ|PC64_WRITE)==0);CHECK(pc64_host_add_region(f.host,f.stack,65536,PC64_READ|PC64_WRITE)==0);atomic_init(&f.record.word,0);
 for(unsigned i=0;i<65;i++)f.values[i]=i+1;
 uint32_t mainTag=0xdeadbeef;pc64_bind_host(f.host,on_fault,&f);NgpFlsCallbackV1 callback={own_callback,&f};NgpFlsReportV1 r=report();
 CHECK(cookie_worker_tag_read_v1(&mainTag)==COOKIE_TAG_UNBOUND_V1&&mainTag==0xdeadbeef);
 CHECK(ngp_fls_init_v1(f.service,ngp_fls_size_v1(),f.host,&callback,&r)==NGP_FLS_OWNER&&!r.worker_observation);
 CHECK(cookie_worker_tag_read_v1(&mainTag)==COOKIE_TAG_UNBOUND_V1&&mainTag==0xdeadbeef);pc64_bind_host(NULL,NULL,NULL);
 pthread_t a,b;CHECK(pthread_create(&a,NULL,auxiliary,NULL)==0);REQUIRE(pthread_mutex_lock(&coordination)==0);while(!aux_ready)REQUIRE(pthread_cond_wait(&condition,&coordination)==0);REQUIRE(pthread_mutex_unlock(&coordination)==0);
 CHECK(pthread_create(&b,NULL,owner,NULL)==0&&pthread_join(b,NULL)==0);
 r=report();CHECK(ngp_fls_observe_v1(f.service,&r)==0&&!r.worker_observation&&!r.token&&!r.boxes_used&&!r.key_created);
 if(!strcmp(f.mode,"owner-exit")||!strcmp(f.mode,"set-fail"))CHECK(r.phase==NGP_FLS_OWNER_EXITED&&r.owner_exit_observed&&!atomic_load_explicit(&f.callbacks,memory_order_relaxed));
 else if(!strcmp(f.mode,"callback-fail")||!strcmp(f.mode,"create-fail")||!strcmp(f.mode,"clear-fail")||!strcmp(f.mode,"delete-fail"))CHECK(r.phase==NGP_FLS_FAILED);
 else CHECK(r.phase==NGP_FLS_CLOSED);
 REQUIRE(pthread_mutex_lock(&coordination)==0);aux_stop=1;REQUIRE(pthread_cond_broadcast(&condition)==0);REQUIRE(pthread_mutex_unlock(&coordination)==0);CHECK(pthread_join(a,NULL)==0);
 printf("PASS authority-join mode=%s cases=%u faults=%u foreignOps=%u callbacks=%u boxes=%u lateEvent=%u tagThreads=2 realHost=1 realDepth=1 ownCallback=1 sourceReady=0\n",f.mode,f.cases,f.faults,atomic_load_explicit(&f.foreign_ops,memory_order_relaxed),atomic_load_explicit(&f.callbacks,memory_order_relaxed),f.boxes,f.late_event);
 return 0; /* Every published native owner/box remains allocated to process exit. */
}
