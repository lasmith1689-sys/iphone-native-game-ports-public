#include "inc32_flags.h"
void pc64_inc32_flags_v1(uint32_t old,Pc64Inc32FlagsV1*out){
 uint32_t result=old+UINT32_C(1);uint32_t p=result&255u;
 p^=p>>4;p^=p>>2;p^=p>>1;
 out->zf=result==0;out->sf=result>>31;out->of=old==UINT32_C(0x7fffffff);
 out->pf=(~p)&1u;out->af=((old^result^UINT32_C(1))&16u)!=0;
}
