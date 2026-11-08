#include "wave_info_pcm.h"
#include "ngp_wave_pcm_v1.h"
_Static_assert(sizeof(CodMilesWaveInfoV1)==36,"exact nine-word result");
int cod_miles_wave_info_pcm_v1(const void *source,size_t bytes,
                              uint32_t guest_address,CodMilesWaveInfoV1 *out){
 if(!source||!out||!bytes||bytes>NGP_WAVE_MAX_BYTES_V1||
    (uintptr_t)source>UINTPTR_MAX-(bytes-1)||
    (uintptr_t)out>UINTPTR_MAX-(sizeof(*out)-1)||
    (uint64_t)guest_address+bytes>UINT64_C(0x100000000))return NGP_WAVE_INVALID_V1;
 if((uintptr_t)source<=(uintptr_t)out+sizeof(*out)-1&&
    (uintptr_t)out<=(uintptr_t)source+bytes-1)return NGP_WAVE_INVALID_V1;
 NgpWavePcmV1 info={0};int status=ngp_wave_pcm_v1_parse(source,bytes,&info);
 if(status)return status;
 if(!info.data_bytes)return NGP_WAVE_INVALID_V1; /* original has no PCM success for empty data */
 uint64_t data=(uint64_t)guest_address+info.data_offset;
 if(data>UINT32_MAX)return NGP_WAVE_INVALID_V1;
 CodMilesWaveInfoV1 value={{info.format_tag,(uint32_t)data,info.data_bytes,
    info.sample_rate,info.bits,info.channels,info.sample_units,info.block_align,
    (uint32_t)data}};
 *out=value;return NGP_WAVE_OK_V1;
}
