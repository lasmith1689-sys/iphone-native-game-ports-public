/* OWN synthetic PCM and serialized graph test. Never opens a device/file or
 * executes a game. A measured no-allocation interval is not a realtime proof. */
#include "mixer_profile.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { VOICES=77, FRAMES=16384, BLOCK=4096 };
typedef struct { size_t allocations,reallocations,frees; int forbid,violation; } allocations;
static allocations counts;
static ma_engine engine;
static ma_sound voices[VOICES];
static ma_audio_buffer buffers[VOICES];
static float pcm[FRAMES*2],output[BLOCK*2];
static void* alloc(size_t n,void* user) { allocations*a=user; a->allocations++; if(a->forbid){a->violation=1;return NULL;} return malloc(n); }
static void* reallocate(void*p,size_t n,void*user) { allocations*a=user; a->reallocations++; if(a->forbid){a->violation=1;return NULL;} return realloc(p,n); }
static void release(void*p,void*user) { allocations*a=user; a->frees++; if(a->forbid){a->violation=1;return;} free(p); }
static void read_frames(unsigned n) {
 ma_uint64 got=0;size_t a=counts.allocations,r=counts.reallocations,f=counts.frees;
 assert(n>0&&n<=BLOCK);memset(output,0x55,sizeof(output));counts.forbid=1;
 ma_result result=ma_engine_read_pcm_frames(&engine,output,n,&got);
 counts.forbid=0;
 assert(result==MA_SUCCESS&&got==n&&!counts.violation);
 assert(a==counts.allocations&&r==counts.reallocations&&f==counts.frees);
 for(unsigned i=0;i<n*2;i++)assert(isfinite(output[i]));
}
static void constant(unsigned frames,float left,float right) {
 for(unsigned i=0;i<frames;i++) {assert(fabsf(output[i*2]-left)<0.00002f);assert(fabsf(output[i*2+1]-right)<0.00002f);}
}
int main(void) {
 ma_allocation_callbacks callbacks;memset(&callbacks,0,sizeof(callbacks));callbacks.pUserData=&counts;callbacks.onMalloc=alloc;callbacks.onRealloc=reallocate;callbacks.onFree=release;
 ma_engine_config ec=ma_engine_config_init();ec.noDevice=MA_TRUE;ec.channels=2;ec.sampleRate=48000;ec.allocationCallbacks=callbacks;
 assert(ma_engine_init(&ec,&engine)==MA_SUCCESS);
 for(unsigned i=0;i<FRAMES;i++){pcm[i*2]=0.001f;pcm[i*2+1]=0.002f;}
 for(unsigned i=0;i<VOICES;i++) {
  ma_audio_buffer_config bc=ma_audio_buffer_config_init(ma_format_f32,2,FRAMES,pcm,&callbacks);bc.sampleRate=48000;
  assert(ma_audio_buffer_init(&bc,&buffers[i])==MA_SUCCESS);
  assert(ma_sound_init_from_data_source(&engine,&buffers[i],0,NULL,&voices[i])==MA_SUCCESS);
  ma_sound_set_spatialization_enabled(&voices[i],MA_FALSE);
  assert(ma_sound_start(&voices[i])==MA_SUCCESS);
 }
 /* Pinned linear resampler starts with one zero history frame even at pitch1.
  * Keep this observable delay in the oracle instead of priming it away. */
 read_frames(128);assert(output[0]==0&&output[1]==0);
 for(unsigned i=1;i<128;i++){assert(fabsf(output[i*2]-VOICES*0.001f)<0.00002f);assert(fabsf(output[i*2+1]-VOICES*0.002f)<0.00002f);}
 ma_uint64 cursor=0;assert(ma_sound_get_cursor_in_pcm_frames(&voices[76],&cursor)==MA_SUCCESS&&cursor>0&&cursor<FRAMES);
 for(unsigned i=1;i<VOICES;i++)assert(ma_sound_stop(&voices[i])==MA_SUCCESS);
 read_frames(256);constant(256,0.001f,0.002f);
 assert(ma_sound_seek_to_pcm_frame(&voices[0],FRAMES-100)==MA_SUCCESS);ma_sound_set_looping(&voices[0],MA_TRUE);
 read_frames(1024);constant(1024,0.001f,0.002f);assert(ma_sound_is_playing(&voices[0]));
 assert(ma_sound_get_cursor_in_pcm_frames(&voices[0],&cursor)==MA_SUCCESS&&cursor<FRAMES);
 assert(ma_sound_stop(&voices[0])==MA_SUCCESS);read_frames(256);constant(256,0,0);
 /* Independent data-source cursor and actual pitch progress at one fixed rate.
  * This does not validate the game's pitch units or a resampling/format cohort. */
 assert(ma_sound_seek_to_pcm_frame(&voices[1],0)==MA_SUCCESS);ma_sound_set_pitch(&voices[1],0.5f);assert(ma_sound_start(&voices[1])==MA_SUCCESS);
 read_frames(2048);ma_uint64 slow=0;assert(ma_sound_get_cursor_in_pcm_frames(&voices[1],&slow)==MA_SUCCESS);assert(ma_sound_stop(&voices[1])==MA_SUCCESS);
 assert(ma_sound_seek_to_pcm_frame(&voices[2],0)==MA_SUCCESS);ma_sound_set_pitch(&voices[2],2.0f);assert(ma_sound_start(&voices[2])==MA_SUCCESS);
 read_frames(2048);ma_uint64 fast=0;assert(ma_sound_get_cursor_in_pcm_frames(&voices[2],&fast)==MA_SUCCESS);assert(slow>0&&fast>slow*2&&fast<FRAMES);
 assert(ma_sound_stop(&voices[2])==MA_SUCCESS);
 for(unsigned i=0;i<VOICES;i++){ma_sound_uninit(&voices[i]);ma_audio_buffer_uninit(&buffers[i]);}
 ma_engine_uninit(&engine);
 printf("{\"voices\":77,\"sumChecks\":384,\"pitchCursorSlow\":%llu,\"pitchCursorFast\":%llu,\"renderAllocationViolation\":false,\"serializedSyntheticOnly\":true,\"AppleAudioOrGameExecuted\":false}\n",(unsigned long long)slow,(unsigned long long)fast);
 return 0;
}
