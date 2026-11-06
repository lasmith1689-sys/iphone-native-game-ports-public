#include "synthetic_profiles.h"
static const Pc64IncSiteV1 sites[4]={
{UINT64_C(0x10001000),UINT64_C(0x10000100),PC64_INC_RAX,3,{0xf0,0xff,0x00}},
{UINT64_C(0x10001010),UINT64_C(0x10000110),PC64_INC_RCX,3,{0xf0,0xff,0x01}},
{UINT64_C(0x10001020),UINT64_C(0x10000120),PC64_INC_RDX,3,{0xf0,0xff,0x02}},
{UINT64_C(0x10001030),UINT64_C(0x10000130),PC64_INC_RAX_15C,7,{0xf0,0xff,0x80,0x5c,0x01,0x00,0x00}},
};
const Pc64IncSiteV1*synthetic_inc_sites_v1(uint32_t*count){*count=4;return sites;}
void synthetic_inc_0(CPU*c){pc64_required_inc32_v1(c,c->rax,UINT64_C(0x10001000));}
void synthetic_inc_1(CPU*c){pc64_required_inc32_v1(c,c->rcx,UINT64_C(0x10001010));}
void synthetic_inc_2(CPU*c){pc64_required_inc32_v1(c,c->rdx,UINT64_C(0x10001020));}
void synthetic_inc_3(CPU*c){pc64_required_inc32_v1(c,c->rax+UINT64_C(0x15c),UINT64_C(0x10001030));}
