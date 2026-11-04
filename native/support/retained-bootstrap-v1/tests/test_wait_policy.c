#include "ngp_boot_wait_v1.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#undef assert
#define assert(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)

int main(void) {
    NgpBootWaitV1 w;
    assert(ngp_boot_wait_begin_v1(&w, 100, 10) == 0);
    assert(ngp_boot_wait_poll_v1(&w, 101, 0, -1) == NGP_BOOT_WAIT_PENDING);
    assert(ngp_boot_wait_poll_v1(&w, 109, 0, 0) == NGP_BOOT_WAIT_LIVE);
    /* A terminal decision cannot be reused to claim a later LIVE result. */
    assert(ngp_boot_wait_poll_v1(&w, 110, 1, -1) == NGP_BOOT_WAIT_LIVE);
    assert(ngp_boot_wait_begin_v1(&w, 100, 10) == 0);
    assert(ngp_boot_wait_poll_v1(&w, 110, 0, 0) == NGP_BOOT_WAIT_TIMEOUT);
    assert(ngp_boot_wait_poll_v1(&w, 109, 0, 0) == NGP_BOOT_WAIT_TIMEOUT);
    assert(ngp_boot_wait_begin_v1(&w, 100, 10) == 0);
    assert(ngp_boot_wait_poll_v1(&w, 101, 1, 0) == NGP_BOOT_WAIT_CANCELLED);
    assert(ngp_boot_wait_begin_v1(&w, 100, 10) == 0);
    assert(ngp_boot_wait_poll_v1(&w, 105, 0, -234) == NGP_BOOT_WAIT_PENDING);
    assert(ngp_boot_wait_poll_v1(&w, 104, 0, 0) == NGP_BOOT_WAIT_CLOCK);
    assert(ngp_boot_wait_begin_v1(&w, UINT64_MAX-10, 10) == 0);
    assert(ngp_boot_wait_poll_v1(&w, UINT64_MAX, 0, 0) == NGP_BOOT_WAIT_TIMEOUT);
    assert(ngp_boot_wait_begin_v1(&w, UINT64_MAX-9, 10) == NGP_BOOT_WAIT_ARGUMENT);
    assert(ngp_boot_wait_begin_v1(&w, 0, 0) == NGP_BOOT_WAIT_ARGUMENT);
    assert(ngp_boot_wait_begin_v1(&w, 0, UINT64_C(60000000001)) == NGP_BOOT_WAIT_ARGUMENT);
    assert(ngp_boot_wait_begin_v1(NULL, 0, 1) == NGP_BOOT_WAIT_ARGUMENT);
    assert(ngp_boot_wait_poll_v1(NULL, 0, 0, 0) == NGP_BOOT_WAIT_ARGUMENT);
    puts("PASS wait policy: pending/live/deadline/cancel/regression/terminal/overflow");
    return 0;
}
