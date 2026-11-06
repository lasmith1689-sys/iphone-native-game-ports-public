/* Own monotone terminal publication.0=PENDING,1=ACK,2=FAILED. */
#include "ngp_audio_stop_terminal_v1.h"
uint32_t ngp_audio_stop_terminal_v1_publish(NgpAudioStopTerminalV1*s,uint32_t value){
 if(!s||(value!=1u&&value!=2u))return 0;
 uint32_t pending=0;
 (void)atomic_compare_exchange_strong_explicit(&s->outcome,&pending,value,memory_order_seq_cst,memory_order_seq_cst);
 return atomic_load_explicit(&s->outcome,memory_order_seq_cst);
}
uint32_t ngp_audio_stop_terminal_v1_read(const NgpAudioStopTerminalV1*s){
 return s?atomic_load_explicit(&s->outcome,memory_order_seq_cst):0;
}
