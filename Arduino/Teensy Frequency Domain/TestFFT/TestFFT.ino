/*
   TestFFT

   Created: Chip Audette  Jan 2017

   Purpose: Generate Output of FFT Routines to confirm that I'm using them
   Correctly

   License:  MIT License

*/

#include <arm_math.h>
#include "rfft.c"

#define N_FFT (256)

arm_cfft_radix4_instance_f32 cfft_inst1, cifft_inst1; //for FFT calls later...radix 4 won't do N=128
//arm_cfft_radix2_instance_f32 cfft_inst1, cifft_inst1; //for FFT calls later...radix 4 won't do N=128
//arm_cfft_radix4_instance_f32 cfft_inst2, cifft_inst2; //for FFT calls later...radix 4 won't do N=128
arm_cfft_radix2_instance_f32 cfft_inst2, cifft_inst2; //for FFT calls later...radix2 will do N=128
arm_rfft_instance_f32 rfft_inst1, rifft_inst1;
//arm_rfft_instance_q31 rfft_inst1;
//arm_cfft_radix4_instance_q31 cfft_inst_q31;
void initFFTroutines(void) {
  int status;
  int N_FFT1 = N_FFT;
  uint8_t ifftFlag = 0; // 0 is FFT, 1 is IFFT
  uint8_t doBitReverse = 1;
  //arm_cfft_radix4_init_f32(&cfft_inst1, N_FFT1, ifftFlag, doBitReverse); //init FFT
  arm_cfft_radix2_init_f32(&cfft_inst2, N_FFT1, ifftFlag, doBitReverse); //init FFT
  //status = arm_rfft_init_q31(&rfft_inst1, &cfft_inst_q31, 128, ifftFlag, doBitReverse); //init FFT
  status = arm_rfft_init_f32(&rfft_inst1, &cfft_inst1, 256, ifftFlag, doBitReverse); //init FFT


  //erial.print("%RFFT Init Status: "); Serial.print(status); Serial.print(", where sucess would be "); Serial.println(ARM_MATH_SUCCESS);
  //delay(1000);
    
  //ifftFlag = 1;
  //arm_cfft_radix4_init_f32(&cifft_inst1, N_FFT1, ifftFlag, doBitReverse); //init FFT
  //arm_cfft_radix2_init_f32(&cifft_inst2, N_FFT1, ifftFlag, doBitReverse); //init FFT
  //arm_rfft_init_f32(&rifft_inst1, &cifft_inst1, N_FFT1, ifftFlag, doBitReverse); //init FFT
}

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("%TestFFT: starting...");
  delay(1000);

  initFFTroutines();
}

void makeSignal(float32_t real_sig[], int Nfft, const float32_t freq_fac, const float32_t DC_offset) {
  for (int i=0; i<2*Nfft;i++) real_sig[i]=0.0f;
  for (int i=0; i<Nfft/2; i++) {
    real_sig[i] = cosf(2.0f*PI*freq_fac*((float32_t)i)/((float32_t)Nfft))+DC_offset;
  }
}

int count = 0;
float32_t real_sig[2*N_FFT];
float32_t complex_sig[2*N_FFT];
int32_t real_sig_q31[2*N_FFT];
int32_t complex_sig_q31[2*N_FFT];
float32_t freq_fac = 10;
float32_t DC_offset = 0.5;
void loop() {
  while (count < 1) {
    count++;

    //create signal
    makeSignal(real_sig, N_FFT, freq_fac, DC_offset);

    //print signal
    Serial.println("%Input Signal: ");
    for (int i=0; i<N_FFT; i++) Serial.println(real_sig[i]);

    //clear the complex signal
    for (int i=0; i< 2*N_FFT; i++) complex_sig[i] = 0.0;

    #if 1
      //do real FFT
      arm_rfft_f32(&rfft_inst1, real_sig, complex_sig);

    #else
      //clear variables
      for (int i=0; i < 2*N_FFT; i++) {
        real_sig_q31[i]=0.0f;
        complex_sig_q31[i]=0.0f;
      }

      //convert to q31
      float_t scale_fac = powf(2.0,16.0);
      for (int i=0; i < N_FFT; i++) {
        real_sig_q31[i]=(int32_t)(scale_fac*real_sig[i]);
      }

      //do q31 fft
      arm_rfft_q31(&rfft_inst1, real_sig_q31, complex_sig_q31);

      //convert back to float
      scale_fac = 1.0/scale_fac;
      for (int i=0; i<2*N_FFT; i++) {
        complex_sig[i] = scale_fac*((float32_t)complex_sig_q31[i]);
      }
    #endif

    Serial.println("%Input Signal Again: ");
    for (int i=0; i<N_FFT; i++) Serial.println(real_sig[i]);

    //print signal
    Serial.println("%Real FFT: ");
    for (int i=0; i< 2*N_FFT; i++) Serial.println(complex_sig[i]);

    //create signal again
    makeSignal(real_sig, N_FFT, freq_fac, DC_offset);

    //copy to teporary vector
    //copy real signal into complex signal
    for (int i=0; i<N_FFT;i++) complex_sig[i] = real_sig[i];    
    cha_fft_rc(complex_sig, N_FFT);

    //print signal
    Serial.println("%CHA FFT: ");
    for (int i=0; i< 2*N_FFT; i++) Serial.println(complex_sig[i]);

    //copy real signal into complex signal
    for (int i=0; i<N_FFT;i++) {
      complex_sig[2*i] = real_sig[i];
      complex_sig[2*i+1] = 0.0f;
    }
    
    //do complex fft
    arm_cfft_radix2_f32(&cfft_inst2,complex_sig);
                    
    //print signal
    Serial.println("%Complex FFT: ");
    for (int i=0; i< 2*N_FFT; i++) Serial.println(complex_sig[i]);

    //end
    Serial.print("%End of Round "); Serial.println(count);
    //delay(1000);
  }


}

