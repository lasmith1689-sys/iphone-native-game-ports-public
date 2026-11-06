#include "inc32_publish.h"
#include "inc32_flags.c"
void pc64_inc32_publish_v1(CPU*cpu,uint64_t returned_old){
 Pc64Inc32FlagsV1 flags;pc64_inc32_flags_v1((uint32_t)returned_old,&flags);
 cpu->zf=flags.zf;cpu->sf=flags.sf;cpu->of=flags.of;cpu->pf=flags.pf;cpu->af=flags.af;
}
