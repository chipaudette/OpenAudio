
#ifndef _DO_ARM_FIR
#define _DO_ARM_FIR

#define ARM_MATH_CM4
#include "arm_math.h"
//include "arm_const_structs.h" //from CMSIS example.  Is it needed?
#include "fft_fir_const.h"



// ///// Choose what type of FIR to do...integer/float

//#define DO_RADIX2 //comment this out to do RADIX4.  RADIX4 is usually faster

#if (DATA_TYPE == USE_INT16)
  #define FIR_DATA_TYPE q15_t
  arm_fir_instance_q15 FIR_inst;
  #define ARM_FIR_INIT_FUNC arm_fir_init_q15
  #define ARM_FIR_FUNC arm_fir_fast_q15

#elif (DATA_TYPE == USE_INT32)

  #define FIR_DATA_TYPE q31_t
  arm_fir_instance_q31 FIR_inst;
  #define ARM_FIR_INIT_FUNC arm_fir_init_q31
  #define ARM_FIR_FUNC arm_fir_q31

#elif (DATA_TYPE == USE_FLOAT)
  #define FIR_DATA_TYPE float32_t
  arm_fir_instance_f32 FIR_inst;
  #define ARM_FIR_INIT_FUNC arm_fir_init_f32
  #define ARM_FIR_FUNC arm_fir_f32

#endif

FIR_DATA_TYPE input[MAX_N];
FIR_DATA_TYPE output[MAX_N];
FIR_DATA_TYPE firCoeff[MAX_N];
FIR_DATA_TYPE firState[MAX_N + MAX_N - 1];

uint32_t start_micros = 0;
uint32_t arm_fir_func(const int N_FIR, const int N_LOOP) {
  if (N_FIR > MAX_N) return 0; // don't ask for N that is too big

  //initialize the FFT function
  //arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);
  ARM_FIR_INIT_FUNC(&FIR_inst, N_FIR, (FIR_DATA_TYPE *)&firCoeff[0], (FIR_DATA_TYPE *)&firState[0],(uint32_t)N_FIR);

  //prepare input
  for (int i=0; i<N_FIR;i++) {
    float val = sinf(2.0*PI*((float32_t)i)/16.0);
    float val2 = cosf(2.0*PI*((float32_t)i)/16.0 + 0.1);
    #if DATA_TYPE == USE_FLOAT
      input[i] = val;
      firCoeff[i]= val2;
    #elif DATA_TYPE == USE_INT16
      input[i] = (q15_t)(255*val);
      firCoeff[i] = (q15_t)(255*val2);
    #elif DATA_TYPE == USE_INT32
      input[i] = (q31_t)(16000*val);
      firCoeff[i] = (q31_t)(16000*val2);
    #endif
  }

  //PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
  start_micros = micros();
  for (int count=0; count < N_LOOP; count++) {

    /* Process the data through the FIR module */
    //arm_fir_f32(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);
    ARM_FIR_FUNC(&FIR_inst,(FIR_DATA_TYPE *)&input[0],(FIR_DATA_TYPE *)&output[0],(uint32_t)N_FIR);
  }

  // return the time it took to do this run
  return (uint32_t)((float)(micros() - start_micros)/((float)N_FIR));
}


#endif
