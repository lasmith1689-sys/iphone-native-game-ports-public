#ifndef NGP_DEVICE_MIXER_RENDER_V1_H
#define NGP_DEVICE_MIXER_RENDER_V1_H
#include "ngp_voice_service_v1.h"
#include <stdatomic.h>
enum {NGP_DEVICE_MIXER_OK_V1=0,NGP_DEVICE_MIXER_SHAPE_V1=1,
 NGP_DEVICE_MIXER_FRAMES_V1=2,NGP_DEVICE_MIXER_BUSY_V1=3,
 NGP_DEVICE_MIXER_SERVICE_V1=4,NGP_DEVICE_MIXER_FAILED_V1=5};
/* Broker-private process-retained state. Initialize once before publishing the
 * callback; channels/service remain immutable. Outer busy includes plane copies.
 * No graph or callback context is freed merely on an idle observation. */
typedef struct {
 NgpVoiceServiceV1 *service;
 uint32_t channels;
 _Atomic uint32_t busy,failed;
 float scratch[NGP_VOICE_MAX_RENDER_FRAMES_V1*2];
} NgpDeviceMixerRenderV1;
/* Valid broker-owned state and valid output/descriptor objects are required.
 * Native address arithmetic checks do not establish arbitrary memory validity.
 * Plane spans must be exclusively caller-owned for the entire callback and
 * disjoint from context, descriptor arrays and the silence scalar.
 * silence is a C scalar local, never the Apple ObjCBool pointer directly. */
int ngp_device_mixer_render_v1(NgpDeviceMixerRenderV1*,float *const*,const size_t*,
 const uint32_t*,uint32_t plane_count,uint32_t frames,uint32_t *silence);
#endif
