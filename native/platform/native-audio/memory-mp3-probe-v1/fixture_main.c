/* OWN complete silence-frame construction; validity is tested, never assumed.
 * No external audio/file/device data. See FIXTURE.md for exact bitfields. */
#include "ngp_memory_mp3_v1.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned checks,decodes;uint64_t frames,samples;int bad_reservoir,main_decode,main_view,main_release,foreign_view,foreign_release,slot65;} Counts;
static Counts counts;
static void check(int ok,const char *label) {counts.checks++;if(!ok){fprintf(stderr,"MP3 own silence assertion: %s\n",label);exit(91);}}
static size_t make(unsigned version,unsigned channels,unsigned frames,int tag,int tail,uint8_t *out,size_t cap) {
 size_t at=0;unsigned i;check(version==1||version==2,"construction version");check(channels==1||channels==2,"construction channels");check(frames==1||frames==2,"construction frames");
 if(tag){check(cap>=1024,"tag capacity");memset(out,0,1024);memcpy(out,"ID3",3);out[3]=3;out[8]=7;out[9]=118;at=1024;}
 for(i=0;i<frames;i++) {
  unsigned samples=version==1?1152u:576u,rate=version==1?44100u:22050u,kbps=version==1?128u:64u,pad=i&1u;
  size_t bytes=(size_t)samples*kbps*125u/rate+pad;check(bytes>=36&&bytes<=cap-at,"frame capacity");memset(out+at,0,bytes);
  out[at]=255;out[at+1]=(uint8_t)(version==1?251:243);out[at+2]=(uint8_t)(((version==1?9u:8u)<<4)|(pad<<1));out[at+3]=(uint8_t)(channels==1?192:0);at+=bytes;
 }
 if(tail){check(cap-at>=128,"tail capacity");memset(out+at,0,128);memcpy(out+at,"TAG",3);at+=128;}
 return at;
}
static void inspect_live(NgpMemoryMp3V1 *owned,unsigned version,unsigned channels,unsigned encoded,size_t source_bytes,int tag,int tail) {
 NgpMemoryMp3ViewV1 v;uint64_t i,expected=(uint64_t)encoded*(version==1?1152u:576u);memset(&v,0x55,sizeof(v));
 check(ngp_memory_mp3_v1_view(owned,&v)==NGP_MP3_OK_V1,"owned view");
 check(v.pcm!=NULL&&v.envelope.pcm_frames==expected&&v.envelope.encoded_frames==encoded,"actual decoded frame counts");
 check(v.envelope.channels==channels&&v.envelope.rate==(version==1?44100u:22050u)&&v.envelope.mpeg_version==version,"actual format");
 check(v.envelope.source_bytes==source_bytes&&v.envelope.tag_prefix_bytes==(tag?1024u:0u)&&v.envelope.tag_suffix_bytes==(tail?128u:0u),"actual source extent/tags");
 check(v.envelope.pcm_bytes==expected*channels*2u&&v.source_end_reached==1&&v.bitstream_integrity_verified==0&&v.gapless_applied==0,"actual bytes/EOF/honest limits");
 for(i=0;i<expected*channels;i++){check(v.pcm[i]==0,"actual silence PCM sample");counts.samples++;}
 counts.frames+=v.envelope.pcm_frames;counts.decodes++;
}
typedef struct {NgpMemoryMp3V1 *owned;int view_code,release_code;int view_unchanged,slot_unchanged;} Foreign;
static void *foreign_worker(void *arg) {
 Foreign *f=(Foreign*)arg;NgpMemoryMp3ViewV1 v,before;NgpMemoryMp3V1 *slot=f->owned;
 memset(&v,0x55,sizeof(v));memcpy(&before,&v,sizeof(v));f->view_code=ngp_memory_mp3_v1_view(f->owned,&v);f->view_unchanged=memcmp(&v,&before,sizeof(v))==0;
 f->release_code=ngp_memory_mp3_v1_release(&slot);f->slot_unchanged=slot==f->owned;return NULL;
}
static void roundtrip(unsigned version,unsigned channels,unsigned frames,int tag,int tail,int foreign) {
 uint8_t source[4096];NgpMemoryMp3V1 *owned=NULL,*busy;size_t n=make(version,channels,frames,tag,tail,source,sizeof(source));
 check(ngp_memory_mp3_v1_decode(source,n,&owned)==NGP_MP3_OK_V1&&owned!=NULL,"actual codec decode");inspect_live(owned,version,channels,frames,n,tag,tail);
 busy=owned;check(ngp_memory_mp3_v1_decode(source,n,&busy)==NGP_MP3_BUSY_V1&&busy==owned,"occupied slot transaction");
 if(foreign){pthread_t t;Foreign f={0};f.owned=owned;check(pthread_create(&t,NULL,foreign_worker,&f)==0,"foreign create");check(pthread_join(t,NULL)==0,"foreign joined before owner release");counts.foreign_view=f.view_code;counts.foreign_release=f.release_code;check(f.view_code==NGP_MP3_WORKER_V1&&f.release_code==NGP_MP3_WORKER_V1&&f.view_unchanged&&f.slot_unchanged,"foreign refuses without mutation");}
 check(ngp_memory_mp3_v1_release(&owned)==NGP_MP3_OK_V1&&owned==NULL,"actual owned release clears slot");check(ngp_memory_mp3_v1_release(&owned)==NGP_MP3_WORKER_V1,"empty release refusal");
}
static void *owner_worker(void *unused) {
 uint8_t source[4096];NgpMemoryMp3V1 *owned=NULL,*slots[64]={0};size_t n;unsigned profile,frames,i;int code;(void)unused;
 check(pthread_main_np()==0,"actual non-main owner");
 for(profile=0;profile<3;profile++)for(frames=1;frames<=2;frames++)roundtrip(profile==2?2u:1u,profile==0?1u:2u,frames,frames==2,frames==2,profile==0&&frames==1);
 for(i=0;i<4;i++)roundtrip(1,2,2,(int)(i&1u),(int)((i>>1)&1u),0);
 n=make(1,2,1,0,0,source,sizeof(source));code=ngp_memory_mp3_v1_decode(source,n-1,&owned);check(code==NGP_MP3_INVALID_V1&&owned==NULL,"truncated transaction");
 source[4]=255;source[5]=128;counts.bad_reservoir=ngp_memory_mp3_v1_decode(source,n,&owned);check(counts.bad_reservoir==NGP_MP3_CODEC_V1&&owned==NULL,"actual codec bad-reservoir refusal transaction");
 code=ngp_memory_mp3_v1_decode((const void*)(uintptr_t)1,(size_t)NGP_MP3_MAX_BYTES_V1+1u,&owned);check(code==NGP_MP3_LIMIT_V1&&owned==NULL,"input cap before dereference");
 n=make(1,2,1,0,0,source,sizeof(source));
 for(i=0;i<64;i++){check(ngp_memory_mp3_v1_decode(source,n,&slots[i])==NGP_MP3_OK_V1,"64 live real decodes");inspect_live(slots[i],1,2,1,n,0,0);}
 counts.slot65=ngp_memory_mp3_v1_decode(source,n,&owned);check(counts.slot65==NGP_MP3_LIMIT_V1&&owned==NULL,"65th refuses");
 for(i=0;i<64;i++)check(ngp_memory_mp3_v1_release(&slots[i])==NGP_MP3_OK_V1&&slots[i]==NULL,"64 owned releases");
 roundtrip(1,2,1,0,0,0);return NULL;
}
int main(void) {
 pthread_t owner;check(pthread_main_np()!=0,"actual main");
 counts.main_decode=ngp_memory_mp3_v1_decode((const void*)(uintptr_t)1,SIZE_MAX,(NgpMemoryMp3V1**)(uintptr_t)1);
 counts.main_view=ngp_memory_mp3_v1_view((const NgpMemoryMp3V1*)(uintptr_t)1,(NgpMemoryMp3ViewV1*)(uintptr_t)1);
 counts.main_release=ngp_memory_mp3_v1_release((NgpMemoryMp3V1**)(uintptr_t)1);
 check(counts.main_decode==NGP_MP3_WORKER_V1&&counts.main_view==NGP_MP3_WORKER_V1&&counts.main_release==NGP_MP3_WORKER_V1,"main refusal before pointer access");
 check(pthread_create(&owner,NULL,owner_worker,NULL)==0,"owner create");check(pthread_join(owner,NULL)==0,"owner joined after releases");
 check(counts.decodes==75&&counts.frames==92736&&counts.samples==182016,"actual aggregate counters");
 printf("{\"format\":\"own-MP3-silence-smoke-v1\",\"successfulDecodes\":%u,\"decodedPCMFrames\":%llu,\"zeroPCMSampleChecks\":%llu,\"checks\":%u,\"mainDecodeCode\":%d,\"mainViewCode\":%d,\"mainReleaseCode\":%d,\"foreignViewCode\":%d,\"foreignReleaseCode\":%d,\"badReservoirCode\":%d,\"slot65Code\":%d,\"silenceOnly\":true,\"bitstreamIntegrityCertified\":false,\"gaplessOrMusicOrStock53Proved\":false,\"deviceAudioOrGameOrPhoneExecuted\":false}\n",counts.decodes,(unsigned long long)counts.frames,(unsigned long long)counts.samples,counts.checks,counts.main_decode,counts.main_view,counts.main_release,counts.foreign_view,counts.foreign_release,counts.bad_reservoir,counts.slot65);
 return 0;
}
