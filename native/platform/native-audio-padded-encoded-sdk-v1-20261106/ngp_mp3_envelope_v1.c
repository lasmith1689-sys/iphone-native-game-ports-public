/* OWN finite envelope arithmetic; payload integrity remains a codec concern. */
#include "ngp_memory_mp3_internal_v1.h"

int ngp_mp3_frame_v1_header(const uint8_t *p,size_t size,NgpMp3FrameV1 *result) {
 static const uint16_t bitrate1[15]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
 static const uint16_t bitrate2[15]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160};
 static const uint32_t rates[3]={44100,48000,32000};
 NgpMp3FrameV1 local={0};uint32_t version,layer,index,frequency,side;
 uint64_t frame_bytes;
 if (!p || !result || size<4) return NGP_MP3_INVALID_V1;
 version=(p[1]>>3)&3u;layer=(p[1]>>1)&3u;index=p[2]>>4;frequency=(p[2]>>2)&3u;
 if (p[0]!=255 || (p[1]&224u)!=224u || version==1 || layer!=1 || index==15 || frequency==3 || (p[3]&3u)==2) return NGP_MP3_INVALID_V1;
 if (version==0 || index==0) return NGP_MP3_UNSUPPORTED_V1;
 local.channels=(p[3]>>6)==3 ? 1u:2u;
 local.version=version==3 ? 1u:2u;
 local.rate=rates[frequency]/(version==3 ? 1u:2u);
 if (!((local.version==1 && local.rate==44100 && (local.channels==1 || local.channels==2)) || (local.version==2 && local.rate==22050 && local.channels==2))) return NGP_MP3_UNSUPPORTED_V1;
 local.bitrate=version==3 ? bitrate1[index]:bitrate2[index];local.samples=version==3 ? 1152u:576u;
 frame_bytes=(uint64_t)local.samples*local.bitrate*125u/local.rate+((p[2]>>1)&1u);
 side=version==3 ? (local.channels==1 ? 17u:32u):(local.channels==1 ? 9u:17u);
 if (frame_bytes<4u+((p[1]&1u) ? 0u:2u)+side || frame_bytes>4096u) return NGP_MP3_INVALID_V1;
 local.frame_bytes=(uint32_t)frame_bytes;*result=local;return NGP_MP3_OK_V1;
}

int ngp_mp3_envelope_v1_scan(const void *source,size_t bytes,NgpMp3EnvelopeV1 *result) {
 const uint8_t *p=(const uint8_t*)source;NgpMp3EnvelopeV1 local={0};
 size_t start=0,end=bytes,at;uint64_t total=0,count=0;NgpMp3FrameV1 first={0},f={0};int status;
 if (!source || !result || bytes<4) return NGP_MP3_INVALID_V1;
 if (bytes>NGP_MP3_MAX_BYTES_V1) return NGP_MP3_LIMIT_V1;
 if ((uintptr_t)source>UINTPTR_MAX-bytes || (uintptr_t)result>UINTPTR_MAX-sizeof(*result)) return NGP_MP3_INVALID_V1;
 if ((uintptr_t)source<(uintptr_t)result+sizeof(*result) && (uintptr_t)result<(uintptr_t)source+bytes) return NGP_MP3_INVALID_V1;
 if (p[0]=='I' && p[1]=='D' && p[2]=='3') {
  uint32_t body;
  if (bytes<10) return NGP_MP3_INVALID_V1;
  if (p[3]!=3 || p[4]!=0 || p[5]!=0) return NGP_MP3_UNSUPPORTED_V1;
  if ((p[6]|p[7]|p[8]|p[9])&128u) return NGP_MP3_INVALID_V1;
  body=((uint32_t)p[6]<<21)|((uint32_t)p[7]<<14)|((uint32_t)p[8]<<7)|p[9];
  if (body>bytes-10u) return NGP_MP3_INVALID_V1;
  start=10u+body;
 }
 if (end-start>=128u && p[end-128]=='T' && p[end-127]=='A' && p[end-126]=='G') {end-=128u;local.tag_suffix_bytes=128;}
 if (end-start<4) return NGP_MP3_INVALID_V1;
 at=start;
 while (at<end) {
  status=ngp_mp3_frame_v1_header(p+at,end-at,&f);
  if (status!=NGP_MP3_OK_V1) return status;
  if (count && (first.version!=f.version || first.rate!=f.rate || first.channels!=f.channels)) return NGP_MP3_UNSUPPORTED_V1;
  if (f.frame_bytes > end - at) return NGP_MP3_INVALID_V1;
  if (!count) first=f;
  count++;total+=f.samples;
  if (count>NGP_MP3_MAX_FRAMES_V1 || total > NGP_MP3_MAX_BYTES_V1 / (f.channels * 2u)) return NGP_MP3_LIMIT_V1;
  at += f.frame_bytes;
 }
 if (at!=end || !count || total>UINT32_MAX || total*first.channels*2u>SIZE_MAX) return NGP_MP3_LIMIT_V1;
 local.source_bytes=(uint32_t)bytes;local.audio_offset=(uint32_t)start;local.audio_bytes=(uint32_t)(end-start);
 local.encoded_frames=(uint32_t)count;local.pcm_frames=(uint32_t)total;local.pcm_bytes=(uint32_t)(total*first.channels*2u);
 local.rate=first.rate;local.channels=first.channels;local.mpeg_version=first.version;local.tag_prefix_bytes=(uint32_t)start;
 *result = local;return NGP_MP3_OK_V1;
}
