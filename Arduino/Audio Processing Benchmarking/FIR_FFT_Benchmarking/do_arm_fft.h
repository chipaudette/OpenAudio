
#ifndef _DO_ARM_FFT
#define _DO_ARM_FFT

#define ARM_MATH_CM4
#include "arm_math.h"
//include "arm_const_structs.h" //from CMSIS example.  Is it needed?
#include "fft_fir_const.h"

//// parameters for stepping through different FFT lengths
//#define MAX_N_FFT 2048
//#define N_ALL_FFT 7
//int ALL_N_FFT[N_ALL_FFT] = {MAX_N_FFT/64, MAX_N_FFT/32, MAX_N_FFT/16, MAX_N_FFT/8, MAX_N_FFT/4, MAX_N_FFT/2, MAX_N_FFT};
//#define N_FFT_LOOP 1000

//parameters for CMSIS FFT functions
uint8_t ifftFlag = 0;
uint8_t doBitReverse = 1;

// ///// Choose what type of FFT to do...integer/float, radix2 or radix4

#define DO_RADIX2 //comment this out to do RADIX4.  RADIX4 is usually faster
//#define DO_RFFT   //enable this to do RFFT instead of CFFT.

#if (DATA_TYPE == USE_INT16)

#ifdef DO_RADIX2
	q15_t buffer_complex[MAX_N*2];
	arm_cfft_radix2_instance_q15 fft_inst;
	#define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_q15
	#define ARM_FFT_FUNC arm_cfft_radix2_q15
#else
	q15_t buffer_complex[MAX_N*2];
	arm_cfft_radix4_instance_q15 fft_inst;
	#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_q15
	#define ARM_FFT_FUNC arm_cfft_radix4_q15
#endif

#elif (DATA_TYPE == USE_INT32)

#ifdef DO_RADIX2
	q31_t buffer_complex[MAX_N*2];
	arm_cfft_radix2_instance_q31 fft_inst;
	#define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_q31
	#define ARM_FFT_FUNC arm_cfft_radix2_q31
#else
	q31_t buffer_complex[MAX_N*2];
	arm_cfft_radix4_instance_q31 fft_inst;
	#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_q31
	#define ARM_FFT_FUNC arm_cfft_radix4_q31
#endif

#elif (DATA_TYPE == USE_FLOAT)

#ifdef DO_RFFT
  //http://www.keil.com/pack/doc/CMSIS/DSP/html/group__RealFFT.html
  float32_t buffer_input[3*MAX_N];
  float32_t buffer_output[3*MAX_N];
  arm_cfft_radix4_instance_f32 cfft_rad4_inst;
  arm_rfft_instance_f32 fft_inst;
  #define ARM_FFT_INIT_FUNC arm_rfft_init_f32
  #define ARM_FFT_FUNC arm_rfft_f32
#else
  #ifdef DO_RADIX2
  	  float32_t buffer_complex[MAX_N*2];
  	  arm_cfft_radix2_instance_f32 fft_inst;
  	  #define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_f32
  	  #define ARM_FFT_FUNC arm_cfft_radix2_f32
  #else
    float32_t buffer_complex[MAX_N*2];
  	arm_cfft_radix4_instance_f32 fft_inst;
  	#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_f32
  	#define ARM_FFT_FUNC arm_cfft_radix4_f32
  #endif
#endif

#endif

uint32_t start_micros = 0;
uint32_t arm_fft_func(const int N_FFT, const int N_LOOP) {
  if (N_FFT > MAX_N) return 0; // don't ask for N that is too big
  if (N_FFT < 32) return 0;  //dont' ask for N that is too small

	//initialize the FFT function
  #ifdef DO_RFFT
    ARM_FFT_INIT_FUNC(&fft_inst, &cfft_rad4_inst, N_FFT, ifftFlag, doBitReverse);
  #else
	  ARM_FFT_INIT_FUNC(&fft_inst, N_FFT, ifftFlag, doBitReverse);
  #endif

	//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
	start_micros = micros();
	for (int count=0; count < N_LOOP; count++) {

    #ifdef DO_RFFT
      // prepare the data...real only
      for (int j=0; j < N_FFT; j++) buffer_input[j] = j % 8;  // 8 sample ramp
  
      /* Process the data through the RFFT module */
      ARM_FFT_FUNC(&fft_inst, buffer_input, buffer_output);
    #else
  		// prepare the data
  		for (int j=0; j < 2*N_FFT; j += 2) {
  			//first sample is real
  			buffer_complex[j] = j % 8;  // 8 sample ramp
  
  			//second sample is imaginary
  			buffer_complex[j+1] = 0;  // always use a real signal
  		}
      /* Process the data through the CFFT/CIFFT module */
      ARM_FFT_FUNC(&fft_inst, buffer_complex);
    #endif
	}

  // return the time it took to do this run
  return micros() - start_micros;
}


#endif
