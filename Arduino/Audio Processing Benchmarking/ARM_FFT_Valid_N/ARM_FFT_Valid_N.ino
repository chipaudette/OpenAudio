

#include <Audio.h>

#define ARM_MATH_CM4
#include "arm_math.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

}

void tryInitFFT(const int N) {
  uint8_t ifftFlag = 0; // 0 is FFT, 1 is IFFT
  uint8_t doBitReverse = 1;
  int status;
  
  //Serial.print("tryInitFFT: N_FFT = "); Serial.print(N);
  
  arm_cfft_radix4_instance_f32 foo1;
  status = arm_cfft_radix4_init_f32(&foo1, N,ifftFlag, doBitReverse);
  Serial.print(N);Serial.print(": arm_cfft_radix4_init_f32, FFT allocation : "); Serial.print(status); Serial.print(", where success is "); Serial.println(ARM_MATH_SUCCESS); 
  
  arm_cfft_radix2_instance_f32 foo2;
  status = arm_cfft_radix2_init_f32(&foo2, N,ifftFlag, doBitReverse);
  Serial.print(N);Serial.print(": arm_cfft_radix2_init_f32, FFT allocation : "); Serial.print(status); Serial.print(", where success is "); Serial.println(ARM_MATH_SUCCESS);  
  
  arm_cfft_radix4_instance_f32 foo3;
  arm_rfft_instance_f32 foo4;
  status = arm_rfft_init_f32(&foo4, &foo3, N, ifftFlag, doBitReverse);
  Serial.print(N);Serial.print(": arm_rfft_init_f32, FFT allocation : "); Serial.print(status); Serial.print(", where success is "); Serial.println(ARM_MATH_SUCCESS); 
}

int count=0;
void loop() {
  // put your main code here, to run repeatedly:

  if (count++ < 2) {
    const int N_N = 5;
    int32_t all_N[] = {32, 64, 128, 256, 512};
    for (int i=0; i < N_N; i++) {
      tryInitFFT(all_N[i]);
    }
    delay(2000);
  }
  
}
