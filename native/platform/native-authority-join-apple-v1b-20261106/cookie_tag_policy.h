#ifndef COOKIE_TAG_POLICY_V1_H
#define COOKIE_TAG_POLICY_V1_H
#include <stdint.h>
/* Pure transition only. Production always passes UINT32_MAX. Small maxima
 * are permitted only for explicitly synthetic, separate allocator fixtures. */
static inline int cookie_tag_next_v1(uint32_t last,uint32_t maximum,uint32_t*next){if(!maximum||last>=maximum)return 0;*next=last+1;return 1;}
#endif
