#include "ngp_wave_pcm_v1.h"
static uint8_t bytes[9000],saved[9000];
static NgpWavePcmV1 info;
static void u16(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void u32(uint8_t*p,uint32_t v){for(uint32_t k=0;k<4;k++)p[k]=(uint8_t)(v>>(8*k));}
static void four(uint8_t*p,const char*s){for(uint32_t k=0;k<4;k++)p[k]=(uint8_t)s[k];}
static int same(const void*a,const void*b,size_t n){const uint8_t*p=a,*q=b;for(size_t i=0;i<n;i++)if(p[i]!=q[i])return 0;return 1;}
static uint32_t fixture(uint32_t channels,uint32_t bits,uint32_t rate,uint32_t frames,uint32_t odd,uint32_t data_first){
 uint8_t*p=bytes+1;for(uint32_t k=0;k<8999;k++)p[k]=0xa5;
 four(p,"RIFF");four(p+8,"WAVE");uint32_t off=12,align=channels*bits/8,n=frames*align;
 if(odd){four(p+off,"JUNK");u32(p+off+4,1);p[off+8]=42;p[off+9]=0;off+=10;}
 uint32_t fmt=off+(data_first?8+n+(n&1u):0),data=off+(data_first?0:24);
 four(p+fmt,"fmt ");u32(p+fmt+4,16);u16(p+fmt+8,1);u16(p+fmt+10,channels);
 u32(p+fmt+12,rate);u32(p+fmt+16,rate*align);u16(p+fmt+20,align);u16(p+fmt+22,bits);
 four(p+data,"data");u32(p+data+4,n);for(uint32_t k=0;k<n;k++)p[data+8+k]=(uint8_t)(k*37u);
 uint32_t size=off+24+8+n+(n&1u);u32(p+4,size-8);return size;
}
static int valid(void){
 const uint32_t rates[]={11025,22050,24000,44100,48000,192000};
 for(uint32_t c=1;c<=2;c++)for(uint32_t b=8;b<=16;b+=8)for(uint32_t r=0;r<6;r++)
 for(uint32_t order=0;order<2;order++)for(uint32_t odd=0;odd<2;odd++)for(uint32_t frames=0;frames<=3;frames++){
  uint32_t n=fixture(c,b,rates[r],frames,odd,order);for(uint32_t k=0;k<9000;k++)saved[k]=bytes[k];
  if(ngp_wave_pcm_v1_parse(bytes+1,n,&info))return 101;
  uint32_t off=12+10*odd+(order?0:24)+8;
  if(info.format_tag!=1||info.data_offset!=off||info.data_bytes!=frames*c*b/8||info.sample_rate!=rates[r]||info.bits!=b||info.channels!=c||info.frames!=frames||info.sample_units!=frames*c||info.block_align!=c*b/8||info.container_bytes!=n)return 102;
  if(!same(bytes,saved,sizeof(bytes)))return 103;
  if(ngp_wave_pcm_v1_parse(bytes+1,n+7,&info)||info.container_bytes!=n)return 104;
 }
 return 0;
}
static int denied(const void*p,size_t n,NgpWavePcmV1*out){
 NgpWavePcmV1 before=info;for(uint32_t k=0;k<9000;k++)saved[k]=bytes[k];
 return !ngp_wave_pcm_v1_parse(p,n,out)||!same(&info,&before,sizeof(info))||!same(bytes,saved,sizeof(bytes));
}
static int invalid(void){
 uint32_t n=fixture(2,16,44100,3,0,0);
 for(uint32_t k=0;k<n;k++)if(denied(bytes+1,k,&info))return 201;
 if(denied(0,n,&info)||denied(bytes+1,n,0)||denied(bytes+1,n,(void*)(bytes+2))||denied((void*)(UINTPTR_MAX-10),44,&info)||denied(bytes+1,NGP_WAVE_MAX_BYTES_V1+1u,&info))return 202;
 const uint32_t offsets[]={0,8,12,20,22,24,28,32,34,40};
 const uint32_t values[]={0,0,0,0x11,3,0,1,1,24,11};
 for(uint32_t k=0;k<10;k++){
  n=fixture(2,16,44100,3,0,0);uint8_t*p=bytes+1;
  if(k==3||k==4||k==7||k==8)u16(p+offsets[k],values[k]);else u32(p+offsets[k],values[k]);
  if(denied(p,n,&info))return 203+(int)k;
 }
 n=fixture(1,8,22050,1,0,0);u32(bytes+5,n-9);if(denied(bytes+1,n,&info))return 220; /* missing odd padding */
 n=fixture(1,16,22050,1,0,0);four(bytes+1+n,"data");u32(bytes+1+n+4,0);u32(bytes+5,n);if(denied(bytes+1,n+8,&info))return 221;
 n=fixture(1,16,22050,1,0,0);four(bytes+1+n,"fmt ");u32(bytes+1+n+4,16);u32(bytes+5,n+16);if(denied(bytes+1,n+24,&info))return 222;
 n=fixture(1,16,22050,1,0,0);four(bytes+1+n,"JUNK");u32(bytes+1+n+4,0xffffffffu);u32(bytes+5,n);if(denied(bytes+1,n+8,&info))return 223;
 /* 1024 total chunks accepted; the next zero-size unknown chunk is denied. */
 n=fixture(1,16,22050,1,0,0);uint32_t base=n;
 for(uint32_t k=0;k<1022;k++){four(bytes+1+n,"JUNK");u32(bytes+1+n+4,0);n+=8;}
 u32(bytes+5,n-8);if(ngp_wave_pcm_v1_parse(bytes+1,n,&info))return 224;
 four(bytes+1+n,"JUNK");u32(bytes+1+n+4,0);n+=8;u32(bytes+5,n-8);if(denied(bytes+1,n,&info))return 225;
 n=fixture(1,16,22050,1,0,0);u32(bytes+5,base-7);if(denied(bytes+1,n,&info))return 226;
 return 0;
}
__declspec(dllexport) int fixture_wave_model_tests(void){int r=valid();return r?r:invalid();}
