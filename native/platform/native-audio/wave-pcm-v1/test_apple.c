/* Own synthetic test entry. No Apple audio, original game code or samples. */
#define __declspec(x) __attribute__((visibility("default")))
#include "test_native.c"
#include <stdio.h>
int main(void){
 int rc=fixture_wave_model_tests();
 printf("{\"format\":\"NGP-WAVE-native-Mac-test-v1\",\"testReturn\":%d,\"configurations\":384,\"positiveParses\":768,\"AppleAudioExecuted\":false,\"gameExecuted\":false}\n",rc);
 return rc?1:0;
}
