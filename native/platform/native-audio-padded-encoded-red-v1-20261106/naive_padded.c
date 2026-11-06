/* TEST ONLY semantic baseline, not production implementation. */
#include "naive_padded.h"
int ngp_encoded_voice_bind_padded_mp3_v1(NgpVoiceServiceV1*s,NgpVoiceHandleV1 h,const void*p,size_t n,NgpEncodedVoiceBindReportV1*r,NgpMp3PaddingV1*out){
 int code=ngp_encoded_voice_bind_v1(s,h,p,n,NGP_ENCODED_MP3_V1,r);
 if(!code)*out=(NgpMp3PaddingV1){(uint32_t)n,0,(uint32_t)n,NGP_MP3_PADDING_NONE_V1};
 return code;
}
