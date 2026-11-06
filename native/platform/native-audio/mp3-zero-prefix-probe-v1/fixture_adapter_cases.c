/* OWN synthetic structure/pointer/output tests, no sourceFS or asset names. */
#include "ngp_mp3_zero_prefix_v1.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static int one(const uint8_t*p,size_t n,int expected,uint32_t offset,uint32_t kind){
 NgpMp3EnvelopeV1 e,before;NgpMp3PaddingV1 m,mb;memset(&e,0xa5,sizeof e);before=e;memset(&m,0xa5,sizeof m);mb=m;
 int status=ngp_mp3_zero_prefix_v1_scan(p,n,&e,&m);
 if(status!=expected)return 1;
 if(status)return memcmp(&e,&before,sizeof e)||memcmp(&m,&mb,sizeof m);
 NgpMp3EnvelopeV1 strict={0};
 return m.source_bytes!=n||m.padding_bytes!=offset||m.suffix_bytes!=n-offset||m.kind!=kind||
  ngp_mp3_envelope_v1_scan(p+offset,n-offset,&strict)||memcmp(&e,&strict,sizeof e);
}
int ngp_mp3_fixture_adapter_cases_v1(void){
 uint8_t b[8192]={0},frame[417]={255,251,144,0};unsigned count=0;
 const unsigned offsets[]={0,1,2,417,522,4093};
 for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];++i){unsigned n=offsets[i];memset(b,0,sizeof b);memcpy(b+n,frame,sizeof frame);if(one(b,n+sizeof frame,0,n,n?1:0))return -1;++count;}
 for(unsigned i=0;i<3;++i){unsigned n=i==0?2:i==1?417:4093;memset(b,0,sizeof b);b[0]=255;memcpy(b+n,frame,sizeof frame);if(one(b,n+sizeof frame,0,n,2))return -2;++count;}
 memset(b,0,sizeof b);memcpy(b+4094,frame,sizeof frame);if(one(b,4094+sizeof frame,1,0,0))return -3;++count;
 memset(b,0,sizeof b);b[2]='X';memcpy(b+3,frame,sizeof frame);if(one(b,3+sizeof frame,1,0,0))return -4;++count;
 memset(b,0,sizeof b);memcpy(b+2,"ID3",3);memcpy(b+12,frame,sizeof frame);if(one(b,12+sizeof frame,1,0,0))return -5;++count;
 memset(b,0,sizeof b);b[0]=255;b[1]=255;memcpy(b+6,frame,sizeof frame);if(one(b,6+sizeof frame,1,0,0))return -6;++count;
 memset(b,0,sizeof b);b[0]=255;b[2]=1;memcpy(b+3,frame,sizeof frame);if(one(b,3+sizeof frame,1,0,0))return -7;++count;
 memset(b,0,sizeof b);memcpy(b+2,frame,sizeof frame);b[5]=2;memcpy(b+2+sizeof frame,frame,sizeof frame);if(one(b,2+sizeof frame*2,1,0,0))return -8;++count;
 memset(b,0,sizeof b);if(one(b,4097,1,0,0))return -9;++count;
 b[0]=255;if(one(b,4097,1,0,0))return -10;++count;
 memset(b,0,sizeof b);b[2]=255;b[3]=251;if(one(b,4,1,0,0))return -11;++count;
 memset(b,0,sizeof b);memcpy(b+2,frame,sizeof frame);if(one(b,2+sizeof frame-1,1,0,0))return -12;++count;
 memset(b,0,sizeof b);memcpy(b,"ID3\3",4);memcpy(b+10,frame,sizeof frame);if(one(b,10+sizeof frame,0,0,0))return -13;++count;
 memset(b,0,sizeof b);memcpy(b+2,frame,sizeof frame);memcpy(b+2+sizeof frame,"TAG",3);if(one(b,2+sizeof frame+128,0,2,1))return -14;++count;
 /* Whole source cap and wrap checks must precede dereferencing these pointers. */
 NgpMp3EnvelopeV1 e,eb;NgpMp3PaddingV1 p,pb;memset(&e,0xa5,sizeof e);eb=e;memset(&p,0xa5,sizeof p);pb=p;
 if(ngp_mp3_zero_prefix_v1_scan(b,(size_t)NGP_MP3_MAX_BYTES_V1+1,&e,&p)!=3||memcmp(&e,&eb,sizeof e)||memcmp(&p,&pb,sizeof p))return -15;++count;
 if(ngp_mp3_zero_prefix_v1_scan((const void*)(UINTPTR_MAX-2),4,&e,&p)!=1||memcmp(&e,&eb,sizeof e)||memcmp(&p,&pb,sizeof p))return -16;++count;
 if(ngp_mp3_zero_prefix_v1_scan(b,417,(NgpMp3EnvelopeV1*)(void*)b,&p)!=1||memcmp(&p,&pb,sizeof p))return -17;++count;
 if(ngp_mp3_zero_prefix_v1_scan(b,417,&e,(NgpMp3PaddingV1*)(void*)&e)!=1||memcmp(&e,&eb,sizeof e))return -18;++count;
 if(ngp_mp3_zero_prefix_v1_scan(b,417,&e,(NgpMp3PaddingV1*)(UINTPTR_MAX-2))!=1||memcmp(&e,&eb,sizeof e))return -19;++count;
 return (int)count;
}
