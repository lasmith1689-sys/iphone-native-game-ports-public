/* Actual sole app broker bridge. No synthetic ready flag or framework bypass. */
#import "ngp_audio_worker_runtime_v1.h"
int ngp_audio_worker_broker_query_v1(void*value,NgpDeviceMixerFactsV1*out){
 if(!value||!out)return NGP_DEVICE_BROKER_ARGUMENT_V1;
 @try{return[(__bridge NGPDeviceMixerBrokerV1*)value copyDeviceFacts:out];}
 @catch(NSException*e){(void)e;return NGP_DEVICE_BROKER_EXCEPTION_V1;}
}
int ngp_audio_worker_broker_request_stop_v1(void*value){
 if(!value)return NGP_DEVICE_BROKER_ARGUMENT_V1;
 @try{[(__bridge NGPDeviceMixerBrokerV1*)value requestStop];return NGP_DEVICE_BROKER_OK_V1;}
 @catch(NSException*e){(void)e;return NGP_DEVICE_BROKER_EXCEPTION_V1;}
}
