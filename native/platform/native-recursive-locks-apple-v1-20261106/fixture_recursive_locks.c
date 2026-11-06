#include "ngp_recursive_locks_v1.h"
#include "ngp_fls_v1.h"
#include "ngp_worker_tag_v1.h"
#include "cookie_worker_tag.h"
#include "synthetic_profiles.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define CHECK(v) do{if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(4);}}while(0)
typedef struct {uint32_t guard;_Atomic uint32_t word;uint32_t after[2];} Operand;
_Static_assert(sizeof(Operand)==16&&offsetof(Operand,word)==4,"real guarded atomic DWORD");
typedef struct {
 void *host,*locks,*fls,*dispatch,*inc;CPU cpu;unsigned char *image,*stack;
 Operand operand;NgpRecursiveLockDescriptorV1 descriptors[17];uint32_t slots,tag,aux_tag,token;
 const Pc64IncSiteV1 *sites;uint32_t site_count;const char *mode;unsigned action,faults;
 int32_t status,fault_status;uint32_t fault_depth;uint64_t remaining;
 NgpRecursiveLocksReportV1 final;_Atomic uint32_t foreign_ops,callbacks;
 jmp_buf landing;int armed;
} Fixture;
static Fixture f;
static pthread_mutex_t coordination=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static unsigned aux_ready,foreign_go,foreign_done,aux_stop;
enum {CONSTRUCT=1,NORMAL=2,CAPACITY=3,FOREIGN=4,FAULT_HELD=5,ENTER=6,LEAVE=7};
static NgpRecursiveLocksReportV1 report(void){NgpRecursiveLocksReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static NgpFlsReportV1 fls_report(void){NgpFlsReportV1 r={0};r.version=1;r.size=sizeof r;return r;}
static int mode(const char*s){return !strcmp(f.mode,s);}
static uint32_t all(void){return (1u<<f.slots)-1u;}
static void fault(void*context,int32_t status,uint64_t address,uint64_t detail){
 (void)address;(void)detail;Fixture*p=context;p->fault_status=status;p->fault_depth=pc64_dispatch_depth();p->remaining=pc64_dispatch_entries_remaining();
 if(!p->armed){fprintf(stderr,"UNEXPECTED fault %d\n",status);exit(5);}longjmp(p->landing,1);
}
static int32_t own_callback(void*context,void*value){
 Fixture*p=context;atomic_fetch_add_explicit(&p->callbacks,1,memory_order_relaxed);
 if(value!=p->locks||pc64_active_host()!=p->host||pc64_dispatch_depth()!=0)return 701;
 NgpRecursiveLocksReportV1 r=report();
 if(ngp_recursive_locks_observe_v1(p->locks,&r)||r.phase!=NGP_LOCK_LIVE_V1||r.worker_observation||r.held_mask)return 702;
 return 0; /* OWN value only, no original callback or guest CPU entry. */
}
#ifdef NGP_LOCK_TESTING
static NgpRecursiveLocksTestStatsV1 stats(void){
 NgpRecursiveLocksTestStatsV1 s={0};s.version=1;s.size=sizeof s;
 CHECK(ngp_recursive_locks_test_stats_v1(f.locks,&f.cpu,&s)==0);return s;
}
static void inject(uint32_t stage,uint32_t extra,uint32_t index){
 CHECK(ngp_recursive_locks_test_fault_v1(f.locks,&f.cpu,f.descriptors[index].identity,(1u<<stage)|extra)==0);
}
#endif
static void enter_once(uint32_t index){NgpRecursiveLocksReportV1 r=report();CHECK(ngp_recursive_locks_enter_v1(f.locks,&f.cpu,f.descriptors[index].identity,&r)==0);}
static void leave_once(uint32_t index){NgpRecursiveLocksReportV1 r=report();CHECK(ngp_recursive_locks_leave_v1(f.locks,&f.cpu,f.descriptors[index].identity,&r)==0);}
static void entry(CPU*cpu){
 CHECK(cpu==&f.cpu&&pc64_dispatch_depth()==1&&pc64_active_host()==f.host);
 NgpRecursiveLocksReportV1 r=report();f.status=0;
 if(f.action==CONSTRUCT){
  for(uint32_t i=0;i<f.slots;i++){
   r=report();f.status=ngp_recursive_locks_construct_v1(f.locks,cpu,f.descriptors[i].identity,&r);f.final=r;
   if(f.status)break;
   CHECK(r.mutex_init_ack_mask==((1u<<(i+1))-1u)&&r.attr_destroy_ack_mask==r.mutex_init_ack_mask);
   CHECK(r.phase==(i+1==f.slots?NGP_LOCK_LIVE_V1:NGP_LOCK_CONSTRUCTING_V1));
  }
  return;
 }
 if(f.action==NORMAL){
  enter_once(0);
  r=report();CHECK(ngp_recursive_locks_enter_v1(f.locks,cpu,f.descriptors[0].identity,&r)==0);
  CHECK(r.held_mask==1&&r.max_held_depth==2);
  synthetic_inc_0(cpu);leave_once(0);leave_once(0);
  for(uint32_t i=1;i<f.slots;i++){enter_once(i);leave_once(i);}
 }else if(f.action==CAPACITY){
  for(uint32_t i=0;i<64;i++)enter_once(0);
  r=report();CHECK(ngp_recursive_locks_enter_v1(f.locks,cpu,f.descriptors[0].identity,&r)==NGP_LOCK_LIMIT_V1&&r.phase==NGP_LOCK_LIVE_V1&&r.max_held_depth==64&&r.held_mask==1);
  for(uint32_t i=0;i<64;i++)leave_once(0);
  r=report();CHECK(ngp_recursive_locks_leave_v1(f.locks,cpu,f.descriptors[0].identity,&r)==NGP_LOCK_STATE_V1&&r.phase==NGP_LOCK_LIVE_V1&&!r.held_mask);
 }else if(f.action==FOREIGN){
  enter_once(0);CHECK(pthread_mutex_lock(&coordination)==0);foreign_go=1;CHECK(pthread_cond_broadcast(&condition)==0);
  while(!foreign_done)CHECK(pthread_cond_wait(&condition,&coordination)==0);
  CHECK(pthread_mutex_unlock(&coordination)==0);leave_once(0);
 }else if(f.action==FAULT_HELD){
  enter_once(0);(void)pc64_required_read(UINT64_C(1),4);CHECK(0);
 }else if(f.action==ENTER){f.status=ngp_recursive_locks_enter_v1(f.locks,cpu,f.descriptors[0].identity,&r);f.final=r;}
 else if(f.action==LEAVE){f.status=ngp_recursive_locks_leave_v1(f.locks,cpu,f.descriptors[0].identity,&r);f.final=r;}
 else CHECK(0);
}
static const pc64_native_entry entries[]={{UINT64_C(0x10000080),entry}};
static void invoke(unsigned action,int increments,int expected_fault){
 CPU before,expected;memcpy(&before,&f.cpu,448);memcpy(&expected,&before,448);f.action=action;
 if(increments){expected.zf=0;expected.sf=0;expected.of=0;expected.pf=1;expected.af=0;}
 CHECK(pc64_dispatch_depth()==0&&pc64_dispatch_bind(f.dispatch,64)==0);f.armed=1;f.fault_status=0;
 if(setjmp(f.landing)==0){dispatch(&f.cpu,UINT64_C(0x10000080));CHECK(!expected_fault);}
 else CHECK(expected_fault);
 f.armed=0;
 if(expected_fault){CHECK(f.fault_status==PC64_RANGE&&f.fault_depth==1&&f.remaining==63);f.faults++;}
 else CHECK(!f.fault_status&&pc64_dispatch_depth()==0&&pc64_dispatch_entries_remaining()==63);
 CHECK(memcmp(&f.cpu,&expected,448)==0);
 /* Only an actual returned frame or this actual outer C fault landing pad
  * authorizes unbind. Faulted source state never resumes or rolls back. */
 pc64_dispatch_unbind_after_stop();CHECK(pc64_dispatch_depth()==0);
 if(increments)CHECK(atomic_load_explicit(&f.operand.word,memory_order_seq_cst)==6&&f.operand.guard==0x13572468&&f.operand.after[0]==0xaabbccdd&&f.operand.after[1]==0x24681357);
}
static void no_owner_fields(const NgpRecursiveLocksReportV1*r){
 CHECK(!r->worker_observation&&!r->slot_count&&!r->attr_init_ack_mask&&!r->type_ack_mask&&!r->mutex_init_ack_mask&&!r->attr_destroy_ack_mask&&!r->mutex_destroy_ack_mask&&!r->held_mask&&!r->uncertain_mask&&!r->max_held_depth&&!r->native_stage&&!r->native_error&&!r->last_identity&&!r->cancel_disable_ack&&!r->secondary_native_stage&&!r->secondary_native_error);
}
static void *auxiliary(void*unused){
 (void)unused;uint32_t tag=0;CHECK(cookie_worker_tag_warm_v1(&tag)==0&&tag);CHECK(pthread_mutex_lock(&coordination)==0);f.aux_tag=tag;aux_ready=1;CHECK(pthread_cond_broadcast(&condition)==0);
 while(!foreign_go&&!aux_stop)CHECK(pthread_cond_wait(&condition,&coordination)==0);
 unsigned run=foreign_go;CHECK(pthread_mutex_unlock(&coordination)==0);
 if(run){
  pc64_bind_host(f.host,NULL,NULL);CPU*poison=(CPU*)(uintptr_t)0x1358;
  for(uint32_t i=0;i<32;i++){
   NgpRecursiveLocksReportV1 r=report();CHECK(ngp_recursive_locks_construct_v1(f.locks,poison,f.descriptors[0].identity,&r)==NGP_LOCK_OWNER_V1);no_owner_fields(&r);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_recursive_locks_enter_v1(f.locks,poison,f.descriptors[0].identity,&r)==NGP_LOCK_OWNER_V1);no_owner_fields(&r);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_recursive_locks_leave_v1(f.locks,poison,f.descriptors[0].identity,&r)==NGP_LOCK_OWNER_V1);no_owner_fields(&r);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_recursive_locks_retire_v1(f.locks,poison,&r)==NGP_LOCK_OWNER_V1);no_owner_fields(&r);atomic_fetch_add_explicit(&f.foreign_ops,1,memory_order_relaxed);
   r=report();CHECK(ngp_recursive_locks_observe_v1(f.locks,&r)==0&&r.phase==NGP_LOCK_LIVE_V1);no_owner_fields(&r);
  }
  pc64_bind_host(NULL,NULL,NULL);CHECK(pthread_mutex_lock(&coordination)==0);foreign_done=1;CHECK(pthread_cond_broadcast(&condition)==0);CHECK(pthread_mutex_unlock(&coordination)==0);
 }
 CHECK(pthread_mutex_lock(&coordination)==0);while(!aux_stop)CHECK(pthread_cond_wait(&condition,&coordination)==0);CHECK(pthread_mutex_unlock(&coordination)==0);return NULL;
}
static uint32_t lost_stage(void){
 if(mode("lost-attrInit"))return NGP_LOCK_ATTR_INIT_V1;
 if(mode("lost-type")||mode("lost-type-and-cleanup"))return NGP_LOCK_TYPE_V1;
 if(mode("lost-mutexInit"))return NGP_LOCK_MUTEX_INIT_V1;
 if(mode("lost-attrDestroy"))return NGP_LOCK_ATTR_DESTROY_V1;
 if(mode("lost-lock"))return NGP_LOCK_ENTER_V1;
 if(mode("lost-unlock"))return NGP_LOCK_LEAVE_V1;
 if(mode("lost-mutexDestroy"))return NGP_LOCK_MUTEX_DESTROY_V1;
 return 0;
}
static void close_fls(void){
 NgpFlsReportV1 r=fls_report();CHECK(ngp_fls_retire_v1(f.fls,f.token,&r)==0&&r.phase==NGP_FLS_CLOSED&&r.value_clear_ack&&r.callback_ack&&r.key_delete_ack);
 CHECK(atomic_load_explicit(&f.callbacks,memory_order_relaxed)==1);
}
static void *owner(void*unused){
 (void)unused;pc64_bind_host(f.host,fault,&f);CHECK(cookie_worker_tag_warm_v1(&f.tag)==0&&f.tag!=f.aux_tag);
 uint32_t alias=0;CHECK(ngp_worker_tag_read_v1(&alias)==0&&alias==f.tag);
 NgpFlsReportV1 fr=fls_report();NgpFlsCallbackV1 cb={own_callback,&f};CHECK(ngp_fls_init_v1(f.fls,ngp_fls_size_v1(),f.host,&cb,&fr)==0);
 CHECK(pc64_inc_binding_init_v1(f.inc,pc64_inc_binding_size_v1(),f.host,&f.cpu,f.sites,f.site_count)==0&&pc64_inc_bind_v1(f.inc)==0);
 CHECK(pc64_dispatch_init(f.dispatch,pc64_dispatch_size(),f.host,entries,1,0,0,NULL,NULL)==0);
 NgpRecursiveLocksReportV1 r=report();
 CHECK(ngp_recursive_locks_init_v1(f.locks,ngp_recursive_locks_size_v1(),f.host,&f.cpu,f.descriptors,0,&r)==NGP_LOCK_ARGUMENT_V1);
 r=report();CHECK(ngp_recursive_locks_init_v1(f.locks,ngp_recursive_locks_size_v1(),f.host,&f.cpu,f.descriptors,17,&r)==NGP_LOCK_ARGUMENT_V1);
 NgpRecursiveLockDescriptorV1 bad[16];memcpy(bad,f.descriptors,sizeof bad);bad[1].identity=bad[0].identity;r=report();CHECK(ngp_recursive_locks_init_v1(f.locks,ngp_recursive_locks_size_v1(),f.host,&f.cpu,bad,f.slots,&r)==NGP_LOCK_ARGUMENT_V1);
 r=report();CHECK(ngp_recursive_locks_init_v1(f.locks,ngp_recursive_locks_size_v1(),f.host,&f.cpu,f.descriptors,f.slots,&r)==0&&r.phase==NGP_LOCK_CONFIGURED_V1&&r.cancel_disable_ack&&!r.mutex_init_ack_mask);
 /* The deliberate bypass-depth mutant fails HERE before any mutex exists. */
 r=report();CHECK(ngp_recursive_locks_enter_v1(f.locks,&f.cpu,f.descriptors[0].identity,&r)==NGP_LOCK_FRAME_V1);
 r=report();CHECK(ngp_recursive_locks_construct_v1(f.locks,&f.cpu,f.descriptors[0].identity,&r)==NGP_LOCK_FRAME_V1);
 uint32_t stage=lost_stage();
#ifdef NGP_LOCK_TESTING
 if(stage>=NGP_LOCK_ATTR_INIT_V1&&stage<=NGP_LOCK_ATTR_DESTROY_V1)inject(stage,mode("lost-type-and-cleanup")?(1u<<NGP_LOCK_CLEANUP_V1):0,2);
#else
 CHECK(stage==0);
#endif
 invoke(CONSTRUCT,0,0);
 if(stage>=NGP_LOCK_ATTR_INIT_V1&&stage<=NGP_LOCK_ATTR_DESTROY_V1){
  CHECK(f.status==NGP_LOCK_RETAINED_V1&&f.final.phase==NGP_LOCK_FAILED_V1&&f.final.native_stage==stage&&f.final.native_error==-9001&&f.final.uncertain_mask==4);
  CHECK(f.final.mutex_destroy_ack_mask==0);
  CHECK(f.final.attr_init_ack_mask==(stage==NGP_LOCK_ATTR_INIT_V1?3u:7u));
  CHECK(f.final.type_ack_mask==(stage<=NGP_LOCK_TYPE_V1?3u:7u));
  CHECK(f.final.mutex_init_ack_mask==(stage==NGP_LOCK_ATTR_DESTROY_V1?7u:3u));
  CHECK(f.final.attr_destroy_ack_mask==((stage==NGP_LOCK_TYPE_V1||stage==NGP_LOCK_MUTEX_INIT_V1)&&!mode("lost-type-and-cleanup")?7u:3u));
  CHECK(f.final.secondary_native_stage==(mode("lost-type-and-cleanup")?NGP_LOCK_CLEANUP_V1:0)&&f.final.secondary_native_error==(mode("lost-type-and-cleanup")?-9002:0));
  r=report();CHECK(ngp_recursive_locks_retire_v1(f.locks,&f.cpu,&r)==NGP_LOCK_STATE_V1&&r.phase==NGP_LOCK_FAILED_V1);goto done;
 }
 CHECK(f.status==0&&f.final.phase==NGP_LOCK_LIVE_V1&&f.final.mutex_init_ack_mask==all());
#ifdef NGP_LOCK_TESTING
 {NgpRecursiveLocksTestStatsV1 s=stats();CHECK(s.calls[NGP_LOCK_MUTEX_INIT_V1]==f.slots);}
#endif
 fr=fls_report();f.token=UINT32_MAX;CHECK(ngp_fls_allocate_v1(f.fls,&f.token,&fr)==0);fr=fls_report();CHECK(ngp_fls_set_v1(f.fls,f.token,f.locks,&fr)==0);
 if(stage==NGP_LOCK_ENTER_V1||stage==NGP_LOCK_LEAVE_V1){
#ifdef NGP_LOCK_TESTING
  if(stage==NGP_LOCK_LEAVE_V1){invoke(ENTER,0,0);CHECK(f.status==0);}inject(stage,0,0);invoke(stage==NGP_LOCK_ENTER_V1?ENTER:LEAVE,0,0);
  CHECK(f.status==NGP_LOCK_RETAINED_V1&&f.final.phase==NGP_LOCK_FAILED_V1&&f.final.native_stage==stage&&f.final.native_error==-9001&&f.final.uncertain_mask==1);
  CHECK(f.final.held_mask==(stage==NGP_LOCK_LEAVE_V1?1u:0u));r=report();CHECK(ngp_recursive_locks_retire_v1(f.locks,&f.cpu,&r)==NGP_LOCK_STATE_V1);goto done;
#else
  CHECK(0);
#endif
 }
 if(mode("held-fault")){
  invoke(FAULT_HELD,0,1);r=report();CHECK(ngp_recursive_locks_retire_v1(f.locks,&f.cpu,&r)==NGP_LOCK_BUSY_V1&&r.phase==NGP_LOCK_LIVE_V1&&r.held_mask==1&&!r.mutex_destroy_ack_mask);f.final=r;goto done;
 }
 if(mode("normal"))invoke(NORMAL,1,0);
 else if(mode("capacity"))invoke(CAPACITY,0,0);
 else if(mode("foreign"))invoke(FOREIGN,0,0);
 else CHECK(stage==NGP_LOCK_MUTEX_DESTROY_V1);
 close_fls();
#ifdef NGP_LOCK_TESTING
 if(stage==NGP_LOCK_MUTEX_DESTROY_V1)inject(stage,0,0);
#endif
 r=report();f.status=ngp_recursive_locks_retire_v1(f.locks,&f.cpu,&r);f.final=r;
 if(stage==NGP_LOCK_MUTEX_DESTROY_V1){
  CHECK(f.status==NGP_LOCK_RETAINED_V1&&r.phase==NGP_LOCK_FAILED_V1&&r.native_stage==stage&&r.native_error==-9001&&r.uncertain_mask==1&&!r.mutex_destroy_ack_mask);
#ifdef NGP_LOCK_TESTING
  {NgpRecursiveLocksTestStatsV1 s=stats();CHECK(s.calls[NGP_LOCK_MUTEX_DESTROY_V1]==1);}
#endif
 }else{
  CHECK(f.status==0&&r.phase==NGP_LOCK_CLOSED_V1&&r.mutex_destroy_ack_mask==all()&&!r.held_mask&&!r.uncertain_mask);
#ifdef NGP_LOCK_TESTING
  {NgpRecursiveLocksTestStatsV1 s=stats();CHECK(s.calls[NGP_LOCK_MUTEX_DESTROY_V1]==f.slots);}
#endif
  r=report();CHECK(ngp_recursive_locks_retire_v1(f.locks,&f.cpu,&r)==0&&r.phase==NGP_LOCK_CLOSED_V1);
 }
done:
 CHECK(pc64_dispatch_depth()==0&&pc64_inc_unbind_after_stop_v1()==0);pc64_bind_host(NULL,NULL,NULL);return NULL;
}
int main(int argc,char**argv){
#if !defined(__APPLE__)
 (void)argc;(void)argv;return 2; /* A Windows syntax object is not Mac execution. */
#endif
 if(argc!=2)return 2;f.mode=argv[1];f.slots=mode("capacity")?16:14;
 CHECK(mode("normal")||mode("capacity")||mode("foreign")||mode("held-fault")||lost_stage());
#ifndef NGP_LOCK_TESTING
 CHECK(!lost_stage());
#endif
 CHECK(ngp_fls_prepare_process_v1()==0);f.host=calloc(1,pc64_host_size());f.locks=calloc(1,ngp_recursive_locks_size_v1());f.fls=calloc(1,ngp_fls_size_v1());f.dispatch=calloc(1,pc64_dispatch_size());f.inc=calloc(1,pc64_inc_binding_size_v1());f.image=calloc(1,8192);f.stack=calloc(1,65536);
 CHECK(f.host&&f.locks&&f.fls&&f.dispatch&&f.inc&&f.image&&f.stack);CHECK((uintptr_t)f.locks%ngp_recursive_locks_alignment_v1()==0);
 CHECK(pc64_host_init(f.host,pc64_host_size())==0&&pc64_host_set_image(f.host,UINT64_C(0x10000000),f.image,8192)==0);
 f.sites=synthetic_inc_sites_v1(&f.site_count);CHECK(f.site_count==4);
 for(uint32_t i=0;i<4;i++){unsigned char*p=f.image+(f.sites[i].sourceVA-UINT64_C(0x10000000));memcpy(p,f.sites[i].originalBytes,f.sites[i].byteCount);CHECK(pc64_host_add_region(f.host,p,f.sites[i].byteCount,PC64_READ)==0);}
 CHECK(pc64_host_add_region(f.host,f.stack,65536,PC64_READ|PC64_WRITE)==0&&pc64_host_add_region(f.host,&f.operand,sizeof f.operand,PC64_READ|PC64_WRITE)==0);
 f.operand.guard=0x13572468;atomic_init(&f.operand.word,5);f.operand.after[0]=0xaabbccdd;f.operand.after[1]=0x24681357;
 memset(&f.cpu,0x6b,448);f.cpu.rip=f.sites[0].expectedRIP;f.cpu.rax=(uint64_t)(uintptr_t)&f.operand.word;f.cpu.rsp=(uint64_t)(uintptr_t)(f.stack+32768);f.cpu.cf=1;f.cpu.mxcsr=0x1f80;
 for(uint32_t i=0;i<17;i++){f.descriptors[i].version=1;f.descriptors[i].size=sizeof f.descriptors[i];f.descriptors[i].identity=UINT64_C(0x20000000)+16*i;}
 NgpRecursiveLocksReportV1 r=report();CHECK(ngp_recursive_locks_observe_v1(f.locks,&r)==NGP_LOCK_STATE_V1);no_owner_fields(&r);
 pthread_t a,b;CHECK(pthread_create(&a,NULL,auxiliary,NULL)==0);CHECK(pthread_mutex_lock(&coordination)==0);while(!aux_ready)CHECK(pthread_cond_wait(&condition,&coordination)==0);CHECK(pthread_mutex_unlock(&coordination)==0);
 CHECK(pthread_create(&b,NULL,owner,NULL)==0&&pthread_join(b,NULL)==0);
 r=report();CHECK(ngp_recursive_locks_observe_v1(f.locks,&r)==0&&r.phase==f.final.phase);no_owner_fields(&r);
 CHECK(pthread_mutex_lock(&coordination)==0);aux_stop=1;CHECK(pthread_cond_broadcast(&condition)==0);CHECK(pthread_mutex_unlock(&coordination)==0);CHECK(pthread_join(a,NULL)==0);
 printf("PASS recursive-lock mode=%s phase=%u slots=%u mutexInit=%u attrDestroy=%u mutexDestroy=%u held=%u uncertain=%u peak=%u foreign=%u faults=%u callbacks=%u primary=%u:%d secondary=%u:%d realHost=1 realDepth=1 tagThreads=2 sourceReady=0\n",f.mode,f.final.phase,f.slots,f.final.mutex_init_ack_mask,f.final.attr_destroy_ack_mask,f.final.mutex_destroy_ack_mask,f.final.held_mask,f.final.uncertain_mask,f.final.max_held_depth,atomic_load_explicit(&f.foreign_ops,memory_order_relaxed),f.faults,atomic_load_explicit(&f.callbacks,memory_order_relaxed),f.final.native_stage,f.final.native_error,f.final.secondary_native_stage,f.final.secondary_native_error);
 return 0; /* Every published owner/sidecar/value remains allocated to exit. */
}
