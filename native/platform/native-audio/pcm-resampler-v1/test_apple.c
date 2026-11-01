/* Own synthetic test entry; no Apple audio or game code. */
#define __declspec(x) __attribute__((visibility("default")))
#include "test_native.c"
#include <stdio.h>
int main(void){
 int rc=fixture_pcm_model_tests();
 printf("{\"format\":\"NGP-PCM-native-Mac-test-v1\",\"testReturn\":%d,\"vectors\":%u,\"wholeAndPartitionedRuns\":%u,\"AppleAudioExecuted\":false,\"gameExecuted\":false}\n",
  rc,(unsigned)(sizeof(vectors)/sizeof(vectors[0])),(unsigned)(2*sizeof(vectors)/sizeof(vectors[0])));
 return rc?1:0;
}
