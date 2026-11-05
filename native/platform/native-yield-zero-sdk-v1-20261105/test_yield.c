#include "yield_zero.h"
#include <pthread.h>
#include <stdio.h>
typedef struct{pthread_mutex_t mutex;pthread_cond_t condition;unsigned ready,go;} Gate;
typedef struct{Gate*gate;unsigned calls;int failed;} Result;
static pthread_t original_worker;
static void *worker(void *value){
 Result*r=value;Gate*g=r->gate;
 if(pthread_equal(pthread_self(),original_worker))r->failed=1;
 if(pthread_mutex_lock(&g->mutex))return(void*)1;
 g->ready++;
 if(pthread_cond_broadcast(&g->condition))return(void*)1;
 while(!g->go)if(pthread_cond_wait(&g->condition,&g->mutex))return(void*)1;
 if(pthread_mutex_unlock(&g->mutex))return(void*)1;
 for(unsigned i=0;i<1024;i++){if(pc_yield_zero_v1())r->failed=1;r->calls++;}
 return 0;
}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 original_worker=pthread_self();Gate gate={0};Result results[2]={{&gate,0,0},{&gate,0,0}};pthread_t threads[2];
 CHECK(pthread_mutex_init(&gate.mutex,0)==0);CHECK(pthread_cond_init(&gate.condition,0)==0);
 for(unsigned i=0;i<2;i++)CHECK(pthread_create(&threads[i],0,worker,&results[i])==0);
 CHECK(!pthread_equal(threads[0],threads[1]));CHECK(pthread_mutex_lock(&gate.mutex)==0);
 while(gate.ready<2)CHECK(pthread_cond_wait(&gate.condition,&gate.mutex)==0);
 gate.go=1;CHECK(pthread_cond_broadcast(&gate.condition)==0);CHECK(pthread_mutex_unlock(&gate.mutex)==0);
 unsigned own_calls=0;for(unsigned i=0;i<1024;i++){CHECK(pc_yield_zero_v1()==0);own_calls++;}
 for(unsigned i=0;i<2;i++){void*out=(void*)1;CHECK(pthread_join(threads[i],&out)==0&&out==0);CHECK(!results[i].failed&&results[i].calls==1024);}
 CHECK(pthread_equal(pthread_self(),original_worker));CHECK(pthread_cond_destroy(&gate.condition)==0);CHECK(pthread_mutex_destroy(&gate.mutex)==0);
 printf("{\"nativeYieldCalls\":%u,\"workerCalls\":%u,\"mainCalls\":%u,\"workers\":2,\"workersSynchronizedAtGate\":true,\"failures\":0}\n",own_calls+results[0].calls+results[1].calls,results[0].calls+results[1].calls,own_calls);
 return 0;
}
