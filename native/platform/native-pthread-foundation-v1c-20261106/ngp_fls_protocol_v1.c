#include "ngp_fls_protocol_v1.h"
int32_t ngp_fls_key_next_v1(uint32_t previous,uint32_t *output)
{if(!output)return NGP_FLS_ARGUMENT;if(previous>=UINT32_MAX-1)return NGP_FLS_LIMIT;*output=previous+1;return 0;}
int32_t ngp_fls_step_v1(NgpFlsPolicyV1 *state,uint32_t event)
{
 if(!state)return NGP_FLS_ARGUMENT;
 if(state->phase==NGP_FLS_INIT&&state->acknowledgements==0&&event==NGP_FLS_ALLOCATED){state->phase=NGP_FLS_LIVE;return 0;}
 if(state->phase==NGP_FLS_LIVE&&state->acknowledgements==0&&event==NGP_FLS_BEGIN_RETIRE){state->phase=NGP_FLS_RETIRING;return 0;}
 if(state->phase==NGP_FLS_RETIRING){
  if(state->acknowledgements==0&&event==NGP_FLS_CLEAR_ACK){state->acknowledgements=1;return 0;}
  if(state->acknowledgements==1&&event==NGP_FLS_CALLBACK_ACK){state->acknowledgements=3;return 0;}
  if(state->acknowledgements==3&&event==NGP_FLS_DELETE_ACK){state->acknowledgements=7;state->phase=NGP_FLS_CLOSED;return 0;}
 }
 if(state->phase>=NGP_FLS_INIT&&state->phase<=NGP_FLS_RETIRING&&event==NGP_FLS_FAIL){state->phase=NGP_FLS_FAILED;return 0;}
 return NGP_FLS_STATE;
}
