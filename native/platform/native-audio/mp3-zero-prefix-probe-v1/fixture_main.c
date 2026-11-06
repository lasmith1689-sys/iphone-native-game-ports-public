/* OWN nonzero synthetic PCM/MP3 fixture. No retail/device/guest calls. */
#include "ngp_memory_mp3_v1.h"
#include "ngp_mp3_zero_prefix_v1.h"
int ngp_mp3_fixture_adapter_cases_v1(void);
#include "fixture_cases.h"
#include "tone_contract.h"
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846
_Static_assert(sizeof(NgpMp3EnvelopeV1)==44,"eleven unsigned32 envelope fields");
#define CHECK(x) do{if(!(x)){fprintf(stderr,"own nonzero MP3 assertion line %u: %s\n",(unsigned)__LINE__,#x);_Exit(91);}}while(0)
typedef struct {double amplitude,rms,mean,wrong,off;} Metrics;
typedef struct {const OwnMp3Case*c;unsigned char*source;int status;unsigned release_calls,owned_null,frames;Metrics metrics[2];} Work;
static int analyse(const int16_t*pcm,unsigned frames,unsigned channels,unsigned rate,Metrics out[2]){
 if(!pcm||frames<=OWN_SKIP_FIRST+OWN_SKIP_LAST||(channels!=1&&channels!=2)||(rate!=44100&&rate!=22050))return 0;
 unsigned n=frames-OWN_SKIP_FIRST-OWN_SKIP_LAST;
 for(unsigned ch=0;ch<channels;ch++){
  double sum=0,square=0,s[3]={0,0,0},c[3]={0,0,0};double hz[3]={ch==0?440:880,ch==0?880:440,OWN_OFF_HZ};
  for(unsigned i=0;i<n;i++){
   double v=pcm[(size_t)(i+OWN_SKIP_FIRST)*channels+ch]/32768.0;sum+=v;square+=v*v;
   for(unsigned k=0;k<3;k++){double phase=2*PI*hz[k]*i/rate;s[k]+=v*sin(phase);c[k]+=v*cos(phase);}
  }
  Metrics m={2*hypot(s[0],c[0])/n,sqrt(square/n),sum/n,2*hypot(s[1],c[1])/n,2*hypot(s[2],c[2])/n};out[ch]=m;
  if(!isfinite(m.amplitude)||!isfinite(m.rms)||!isfinite(m.mean)||!isfinite(m.wrong)||!isfinite(m.off)||m.amplitude<OWN_AMP_MIN||m.amplitude>OWN_AMP_MAX||m.rms<OWN_RMS_MIN||m.rms>OWN_RMS_MAX||fabs(m.mean)>OWN_MEAN_MAX||m.wrong>OWN_WRONG_MAX||m.off>OWN_OFF_MAX)return 0;
 }
 return 1;
}
static void *worker(void*arg){
 Work*w=arg;CHECK(!pthread_main_np());CHECK(ngp_mp3_fixture_adapter_cases_v1()==26);NgpMemoryMp3V1*owned=NULL;NgpMp3EnvelopeV1 e,before;NgpMp3PaddingV1 padding,padBefore;memset(&padding,0xa5,sizeof padding);padBefore=padding;
 memset(&e,0xa5,sizeof(e));before=e;w->status=ngp_mp3_zero_prefix_v1_scan(w->source,w->c->bytes,&e,&padding);
 if(!w->c->positive){
  CHECK(w->status==NGP_MP3_UNSUPPORTED_V1&&memcmp(&e,&before,sizeof(e))==0&&memcmp(&padding,&padBefore,sizeof padding)==0);
  CHECK(ngp_mp3_zero_prefix_v1_decode(w->source,w->c->bytes,&owned,&padding)==NGP_MP3_UNSUPPORTED_V1&&owned==NULL&&memcmp(&padding,&padBefore,sizeof padding)==0);w->owned_null=1;return NULL;
 }
 CHECK(w->status==0&&e.source_bytes==w->c->encodedBytes&&padding.source_bytes==w->c->bytes&&padding.padding_bytes==w->c->padding&&padding.kind==w->c->paddingKind&&padding.suffix_bytes==w->c->encodedBytes&&e.audio_offset==0&&e.audio_bytes==e.source_bytes&&e.tag_prefix_bytes==0&&e.tag_suffix_bytes==0);
 CHECK(e.rate==w->c->rate&&e.channels==w->c->channels&&e.mpeg_version==w->c->version&&e.encoded_frames==w->c->frames&&e.pcm_frames==w->c->pcmFrames&&e.pcm_bytes==e.pcm_frames*e.channels*2u);
 CHECK(ngp_mp3_zero_prefix_v1_decode(w->source,w->c->bytes,&owned,&padding)==0&&owned!=NULL);
 NgpMemoryMp3ViewV1 v;CHECK(ngp_memory_mp3_v1_view(owned,&v)==0&&v.pcm&&memcmp(&v.envelope,&e,sizeof(e))==0);
 CHECK(v.source_end_reached==1&&v.bitstream_integrity_verified==0&&v.gapless_applied==0);
 CHECK(analyse(v.pcm,e.pcm_frames,e.channels,e.rate,w->metrics));w->frames=e.pcm_frames;
 NgpMemoryMp3V1*baseline=NULL;CHECK(ngp_memory_mp3_v1_decode(w->source+w->c->padding,w->c->encodedBytes,&baseline)==0&&baseline);NgpMemoryMp3ViewV1 bv;CHECK(ngp_memory_mp3_v1_view(baseline,&bv)==0&&bv.pcm&&memcmp(&bv.envelope,&e,sizeof e)==0&&memcmp(bv.pcm,v.pcm,e.pcm_bytes)==0);CHECK(ngp_memory_mp3_v1_release(&baseline)==0&&baseline==NULL);
 w->release_calls+=2;CHECK(ngp_memory_mp3_v1_release(&owned)==0&&owned==NULL);w->owned_null=1;return NULL;
}
static int nonce(const char*p){if(strlen(p)!=32)return 0;for(unsigned i=0;i<32;i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return 0;return 1;}
int main(int argc,char**argv){
 CHECK(pthread_main_np());
 if(argc==3&&strcmp(argv[1],"zero-oracle")==0){
  CHECK(nonce(argv[2]));static const int16_t zero[16384]={0};Metrics m[2]={{0}};
  /* Real analysis of own zeros must fail with91. A bad accepting oracle
   * returns0 so the runner REFUSES this diagnostic before codec positives. */
  if(!analyse(zero,16384,1,44100,m)){fprintf(stderr,"own zero spectral oracle refused\n");return 91;}return 0;
 }
 CHECK(argc==4&&nonce(argv[3]));const OwnMp3Case*c=NULL;for(unsigned i=0;i<10;i++)if(strcmp(argv[1],own_cases[i].name)==0)c=&own_cases[i];CHECK(c);
 FILE*f=fopen(argv[2],"rb");CHECK(f);unsigned char*raw=calloc(1,c->bytes);CHECK(raw&&fread(raw+c->padding,1,c->encodedBytes,f)==c->encodedBytes);if(c->paddingKind==2)raw[0]=255;CHECK(fgetc(f)==EOF&&!ferror(f)&&fclose(f)==0);
 Work w={0};w.c=c;w.source=raw;pthread_t t;CHECK(pthread_create(&t,NULL,worker,&w)==0&&pthread_join(t,NULL)==0);free(raw);CHECK(w.owned_null&&w.release_calls==2u*(unsigned)c->positive);
 printf("{\"format\":\"own-nonzero-MP3-v1\",\"nonce\":\"%s\",\"case\":\"%s\",\"status\":%d,\"rate\":%u,\"channels\":%u,\"decodedFrames\":%u,\"releaseCalls\":%u,\"paddingBytes\":%u,\"paddingKind\":%u,\"strictBaselinePCMEqual\":%s,\"structuralCases\":26,\"ownedResultNull\":true,\"profileRefused\":%s,\"amplitude\":[%.9g,%.9g],\"rms\":[%.9g,%.9g],\"mean\":[%.9g,%.9g],\"wrong\":[%.9g,%.9g],\"off\":[%.9g,%.9g],\"skipFirst\":%u,\"skipLast\":%u,\"gaplessOrIntegrityOrMusicCohortOrStock53Proof\":false,\"AppleAudioOrGameExecuted\":false}\n",argv[3],c->name,w.status,c->rate,c->channels,w.frames,w.release_calls,c->padding,c->paddingKind,c->positive?"true":"false",c->positive?"false":"true",w.metrics[0].amplitude,w.metrics[1].amplitude,w.metrics[0].rms,w.metrics[1].rms,w.metrics[0].mean,w.metrics[1].mean,w.metrics[0].wrong,w.metrics[1].wrong,w.metrics[0].off,w.metrics[1].off,(unsigned)OWN_SKIP_FIRST,(unsigned)OWN_SKIP_LAST);
 CHECK(fflush(stdout)==0);return 0;
}
