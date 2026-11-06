#ifndef NGP_RECURSIVE_LOCK_POLICY_V1_H
#define NGP_RECURSIVE_LOCK_POLICY_V1_H
#include <stdint.h>
#define NGP_LOCK_SLOT_CAPACITY_V1 16u
#define NGP_LOCK_RECURSION_LIMIT_V1 64u
#define NGP_LOCK_COMPLETE_V1(a,t,m,d,w) ((w)!=0u&&(a)==(w)&&(t)==(w)&&(m)==(w)&&(d)==(w))
#define NGP_LOCK_ENTER_ALLOWED_V1(n) ((uint32_t)(n)<NGP_LOCK_RECURSION_LIMIT_V1)
#define NGP_LOCK_LEAVE_ALLOWED_V1(n) ((uint32_t)(n)>0u&&(uint32_t)(n)<=NGP_LOCK_RECURSION_LIMIT_V1)
#define NGP_LOCK_ATTR_CLEANUP_V1(initialized,attempted,bit) (((initialized)&(bit))&&!((attempted)&(bit)))
#endif
