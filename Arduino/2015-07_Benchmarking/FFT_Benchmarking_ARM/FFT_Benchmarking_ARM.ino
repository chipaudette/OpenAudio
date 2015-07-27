
//CMSIS DSP Software library for  ARM (basis of Teensy 3.1 arm_math library
http://www.keil.com/pack/doc/CMSIS/DSP/html/modules.html
#include "arm_math.h"

/* ------------------------------------------------------------------
* Global variables for FFT Bin Example
* ------------------------------------------------------------------- */
#define MAX_N_FFT 4096
#define N_ALL_N_FFT 8
int all_N_FFT[N_ALL_N_FFT] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
//#define N_ALL_N_FFT 3
//int all_N_FFT[N_ALL_N_FFT] = {64, 256, 1024};
int N_FFT;
//q15_t buffer_real[MAX_N_FFT*2];
q15_t buffer_complex[MAX_N_FFT*2];
//q31_t buffer_real[MAX_N_FFT*2];
//q31_t buffer_complex[MAX_N_FFT*2];
//float32_t buffer_real[MAX_N_FFT*2];
//float32_t buffer_complex[MAX_N_FFT*2];
uint8_t ifftFlag = 0;
uint8_t doBitReverse = 1;
arm_cfft_radix2_instance_q15 cfft_inst;
//arm_cfft_radix2_instance_q31 cfft_inst;
//arm_cfft_radix4_instance_q15 cfft_inst;
//arm_rfft_instance_q15 rfft_inst;
//arm_cfft_radix2_instance_f32 cfft_inst;
//arm_cfft_radix4_instance_f32 cfft_inst;
//arm_rfft_instance_f32 rfft_inst;


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting FFT_Benchmarking_ARM...");
}


#define N_TRIALS_PER_FFT 3
#define N_LOOP 200
int32_t accumulated_micros=0;
int32_t start_micros = 0;

void loop() {
  for (int I_FFT_size_ind = 0; I_FFT_size_ind < N_ALL_N_FFT; I_FFT_size_ind++) {
    N_FFT = all_N_FFT[I_FFT_size_ind];
    
    //initialize the FFT function
    arm_cfft_radix2_init_q15(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix2_init_q31(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix4_init_q15(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix4_init_q31(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix4_init_q15(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_rfft_init_q15(&rfft_inst, &cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix4_init_q31(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_rfft_init_q31(&rfft_inst, &cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix2_init_f32(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_cfft_radix4_init_f32(&cfft_inst, N_FFT, ifftFlag, doBitReverse);
    //arm_rfft_init_f32(&rfft_inst, &cfft_inst, N_FFT, ifftFlag, doBitReverse);
    
    //loop over number of trials per FFT
    for (int I_trial = 0; I_trial < N_TRIALS_PER_FFT; I_trial++) {
     
      //reset the timer
      accumulated_micros = 0;
    
      //loop over the desried number of FFTs
      for (int i=0; i < N_LOOP; i++) {
    
        // prepare the data
        //for (int j=0; j < N_FFT; j++) buffer_real[j]=j%8;// 8 sample ramp
        for (int j=0; j < 2*N_FFT; j += 2) {
          //first sample is real
          buffer_complex[j] = j % 8;  // 8 sample ramp
    
          //second sample is imaginary
          buffer_complex[j+1] = 0;  // always use a real signal
        }
    
        // start the clock
        start_micros = micros();
    
        /* Process the data through the CFFT/CIFFT module */
        // //arm_cfft_f32(&arm_cfft_sR_f32_len1024, testInput_f32_10khz, ifftFlag, doBitReverse);
        // //arm_cfft_radix2_q15(fft_instance,input_data)
        // //arm_cfft_q31(fft_instance,input_complex_Q31,ifftFlag,bitReverseFlag);
        // //if (window) apply_window_to_fft_buffer(buffer, window);
        // //arm_cfft_radix4_q15(&fft_inst, buffer_complex_Q15);
        arm_cfft_radix2_q15(&cfft_inst, buffer_complex);
        //arm_cfft_radix2_q31(&cfft_inst, buffer_complex);
        //arm_cfft_radix4_q15(&cfft_inst, buffer_complex);
        //arm_cfft_radix4_q31(&cfft_inst, buffer_complex);
        //arm_rfft_q15(&rfft_inst, buffer_real, buffer_complex);
        //arm_rfft_q31(&rfft_inst, buffer_real, buffer_complex);
        //arm_cfft_radix2_f32(&cfft_inst, buffer_complex);
        //arm_cfft_radix4_f32(&cfft_inst, buffer_complex);
        //arm_rfft_f32(&rfft_inst, buffer_real, buffer_complex);
      
        // add time for this run
        accumulated_micros += (micros() - start_micros);
    
      }
    
      Serial.print("N_FFT = ");
      Serial.print(N_FFT);
      Serial.print(", N_LOOP = ");
      Serial.print(N_LOOP);
      Serial.print(", Micros per FFT = ");
      Serial.print((float)accumulated_micros/((float)N_LOOP));
      Serial.println();
    }
  }
}


