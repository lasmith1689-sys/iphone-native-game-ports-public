/* OWN included worker implementation; never compile as another full codec TU. */
#ifndef NGP_AUDIO_SHARED_IMPLEMENTATION_V1
#error "Include after the one reviewed shared dependency implementation"
#endif
#if !defined(__APPLE__) || !defined(ma_dr_mp3_h) || defined(MA_NO_MP3) || defined(MA_NO_DECODING) || defined(MA_DR_MP3_FLOAT_OUTPUT)
#error "Requires Apple worker and exact enabled embedded S16 MP3 frame decoder"
#endif
#include "ngp_memory_mp3_internal_v1.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
 ma_dr_mp3dec decoder;
 ma_int16 scratch[MA_DR_MP3_MAX_SAMPLES_PER_FRAME];
} NgpMp3CodecV1;
_Static_assert(sizeof(NgpMp3CodecV1) <= 32768,"bounded fixed codec context");
_Static_assert(MA_DR_MP3_MAX_SAMPLES_PER_FRAME==2304,"actual decoder scratch bound");
_Static_assert(sizeof(ma_int16)==sizeof(int16_t),"exact signed16 width");
struct NgpMemoryMp3V1 { NgpMemoryMp3ViewV1 view; };
static _Thread_local NgpMemoryMp3V1 *ngp_mp3_live[NGP_MP3_MAX_LIVE_V1];
static _Thread_local size_t ngp_mp3_live_bytes;

static int ngp_mp3_find(const NgpMemoryMp3V1 *candidate) {
 unsigned i;
 if (!candidate) return -1;
 for (i=0;i<NGP_MP3_MAX_LIVE_V1;i++) if (candidate == ngp_mp3_live[i]) return (int)i;
 return -1;
}

int ngp_memory_mp3_v1_decode(const void *source,size_t bytes,NgpMemoryMp3V1 **result) {
 const uint8_t *p=(const uint8_t*)source;NgpMp3EnvelopeV1 envelope;
 NgpMemoryMp3V1 *owned=NULL;NgpMp3CodecV1 *codec=NULL;int16_t *pcm=NULL;
 size_t at,end;uint64_t frames=0;uint32_t count=0;int status,slot=-1;unsigned i;
 if (pthread_main_np() != 0) return NGP_MP3_WORKER_V1;
 if (!source || !result || bytes<4) return NGP_MP3_INVALID_V1;
 if (bytes>NGP_MP3_MAX_BYTES_V1) return NGP_MP3_LIMIT_V1;
 if ((uintptr_t)source>UINTPTR_MAX-bytes || (uintptr_t)result>UINTPTR_MAX-sizeof(*result)) return NGP_MP3_INVALID_V1;
 if ((uintptr_t)source<(uintptr_t)result+sizeof(*result) && (uintptr_t)result<(uintptr_t)source+bytes) return NGP_MP3_INVALID_V1;
 if (*result) return NGP_MP3_BUSY_V1;
 for (i=0;i<NGP_MP3_MAX_LIVE_V1;i++) if (!ngp_mp3_live[i]) {slot=(int)i;break;}
 if (slot<0) return NGP_MP3_LIMIT_V1;
 status=ngp_mp3_envelope_v1_scan(source,bytes,&envelope);
 if (status!=NGP_MP3_OK_V1) return status;
 if (envelope.pcm_bytes > NGP_MP3_MAX_BYTES_V1 - ngp_mp3_live_bytes) return NGP_MP3_LIMIT_V1;
 owned=(NgpMemoryMp3V1*)calloc(1,sizeof(*owned));
 if (!owned) return NGP_MP3_ALLOCATION_V1;
 codec=(NgpMp3CodecV1*)calloc(1,sizeof(*codec));
 if (!codec) {status=NGP_MP3_ALLOCATION_V1;goto fail;}
 pcm=(int16_t*)malloc(envelope.pcm_bytes);
 if (!pcm) {status=NGP_MP3_ALLOCATION_V1;goto fail;}
 ma_dr_mp3dec_init(&codec->decoder);
 at=envelope.audio_offset;end=at+envelope.audio_bytes;
 while (at<end) {
  NgpMp3FrameV1 f;ma_dr_mp3dec_frame_info info={0};int samples;size_t copy_bytes;
  status=ngp_mp3_frame_v1_header(p+at,end-at,&f);
  if (status!=NGP_MP3_OK_V1) goto fail;
  if (f.frame_bytes>end-at || f.frame_bytes>4096u || f.channels!=envelope.channels || f.rate!=envelope.rate || f.version!=envelope.mpeg_version) {status=NGP_MP3_CODEC_V1;goto fail;}
  samples=ma_dr_mp3dec_decode_frame(&codec->decoder,p+at,(int)f.frame_bytes,codec->scratch,&info);
  if (samples != (int)f.samples || info.frame_bytes != (int)f.frame_bytes || info.channels != (int)f.channels || info.sample_rate != (int)f.rate || info.layer != 3 || info.bitrate_kbps!=(int)f.bitrate) {status=NGP_MP3_CODEC_V1;goto fail;}
  if (frames>envelope.pcm_frames || f.samples>envelope.pcm_frames-frames || count>=envelope.encoded_frames) {status=NGP_MP3_CODEC_V1;goto fail;}
  copy_bytes=(size_t)f.samples*f.channels*sizeof(int16_t);
  memcpy(pcm+(size_t)frames*f.channels,codec->scratch,copy_bytes);
  frames+=f.samples;count++;
  at += f.frame_bytes;
 }
 if (at != end || count!=envelope.encoded_frames || frames!=envelope.pcm_frames || frames*envelope.channels*sizeof(int16_t)!=envelope.pcm_bytes) {status=NGP_MP3_CODEC_V1;goto fail;}
 free(codec);codec=NULL;
 owned->view.pcm=pcm;owned->view.envelope=envelope;owned->view.source_end_reached=1;
 owned->view.bitstream_integrity_verified = 0;
 owned->view.gapless_applied = 0;
 ngp_mp3_live[slot] = owned;ngp_mp3_live_bytes+=envelope.pcm_bytes;
 *result = owned;return NGP_MP3_OK_V1;
fail:
 free(pcm);free(codec);free(owned);return status;
}

int ngp_memory_mp3_v1_view(const NgpMemoryMp3V1 *owned,NgpMemoryMp3ViewV1 *result) {
 if (pthread_main_np() != 0) return NGP_MP3_WORKER_V1;
 if (!result) return NGP_MP3_INVALID_V1;
 if (ngp_mp3_find(owned)<0) return NGP_MP3_WORKER_V1;
 *result=owned->view;return NGP_MP3_OK_V1;
}

int ngp_memory_mp3_v1_release(NgpMemoryMp3V1 **result) {
 NgpMemoryMp3V1 *owned;int slot;
 if (pthread_main_np() != 0) return NGP_MP3_WORKER_V1;
 if (!result) return NGP_MP3_INVALID_V1;
 owned=*result;slot=ngp_mp3_find(owned);
 if (slot<0) return NGP_MP3_WORKER_V1;
 ngp_mp3_live_bytes-=owned->view.envelope.pcm_bytes;ngp_mp3_live[slot]=NULL;
 free((void*)owned->view.pcm);free(owned);*result=NULL;return NGP_MP3_OK_V1;
}
