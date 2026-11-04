#include "ngp_boot_wait_v1.h"
int ngp_boot_wait_begin_v1(NgpBootWaitV1 *w,uint64_t n,uint64_t t) {
 if(!w)return NGP_BOOT_WAIT_ARGUMENT;
 *w=(NgpBootWaitV1){n,n,NGP_BOOT_WAIT_ARGUMENT};
 if(!t||t>UINT64_C(60000000000)||n>UINT64_MAX-t)return w->decision;
 w->deadline_ns=n+t;w->decision=NGP_BOOT_WAIT_PENDING;return 0;
}
int ngp_boot_wait_poll_v1(NgpBootWaitV1 *w,uint64_t n,int c,int s) {
 if(!w)return NGP_BOOT_WAIT_ARGUMENT;
 if(w->decision!=NGP_BOOT_WAIT_PENDING)return w->decision;
 if(c)w->decision=NGP_BOOT_WAIT_CANCELLED;
 else if(n<w->last_ns)w->decision=NGP_BOOT_WAIT_CLOCK;
 else if(n>=w->deadline_ns)w->decision=NGP_BOOT_WAIT_TIMEOUT;
 else if(!s)w->decision=NGP_BOOT_WAIT_LIVE;
 w->last_ns=n;return w->decision;
}
