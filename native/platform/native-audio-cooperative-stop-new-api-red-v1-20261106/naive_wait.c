/* TEST ONLY: deliberately insufficient external wait adapter. This supplies a
 * real semantic wrong baseline for new API tests, never production behavior. */
#include "test_wait_api.h"
int ngp_audio_worker_v1_wait_stop(NgpAudioWorkerRuntimeV1*s,NgpAudioWorkerStopReportV1*r){
 uint64_t start=0,now=0;int rc=ngp_test_wait_now_v1(&start);if(rc)return rc;
 rc=ngp_audio_worker_v1_request_stop(s,r);
 for(unsigned i=0;i<4096&&rc==NGP_AUDIO_WORKER_PENDING;i++){
  if(ngp_test_wait_now_v1(&now))return 9;
  if(now<start||now-start>=UINT64_C(5000000000)){r->status=9;return 9;}
  rc=ngp_audio_worker_v1_poll_stop(s,r);
 }
 return rc;
}
