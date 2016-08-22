
#ifndef _DO_ARM_FFT
#define _DO_ARM_FFT

#define ARM_MATH_CM4
#include "arm_math.h"
#include "arm_const_structs.h" //from CMSIS example.  Is it needed?

// parameters for stepping through different FFT lengths
#define MAX_N_FFT 2048
#define N_ALL_FFT 7
int ALL_N_FFT[N_ALL_FFT] = {MAX_N_FFT/64, MAX_N_FFT/32, MAX_N_FFT/16, MAX_N_FFT/8, MAX_N_FFT/4, MAX_N_FFT/2, MAX_N_FFT};
#define N_FFT_LOOP 1000

//parameters for CMSIS FFT functions
uint8_t ifftFlag = 0;
uint8_t doBitReverse = 1;


// ///// Choose what type of FFT to do...integer/float, radix2 or radix4

//q15_t buffer_complex[MAX_N_FFT*2];
//arm_cfft_radix2_instance_q15 cfft_inst;
//#define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_q15
//#define ARM_FFT_FUNC arm_cfft_radix2_q15

//q15_t buffer_complex[MAX_N_FFT*2];
//arm_cfft_radix4_instance_q15 cfft_inst;
//#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_q15
//#define ARM_FFT_FUNC arm_cfft_radix4_q15

//q31_t buffer_complex[MAX_N_FFT*2];
//arm_cfft_radix2_instance_q31 cfft_inst;
//#define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_q31
//#define ARM_FFT_FUNC arm_cfft_radix2_q31

//q31_t buffer_complex[MAX_N_FFT*2];
//arm_cfft_radix4_instance_q31 cfft_inst;
//#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_q31
//#define ARM_FFT_FUNC arm_cfft_radix4_q31

float32_t buffer_complex[MAX_N_FFT*2];
arm_cfft_radix2_instance_f32 cfft_inst;
#define ARM_FFT_INIT_FUNC arm_cfft_radix2_init_f32
#define ARM_FFT_FUNC arm_cfft_radix2_f32

//float32_t buffer_complex[MAX_N_FFT*2];
//arm_cfft_radix4_instance_f32 cfft_inst;
//#define ARM_FFT_INIT_FUNC arm_cfft_radix4_init_f32
//#define ARM_FFT_FUNC arm_cfft_radix4_f32


int arm_fft_func(void) {
	int N_FFT;
	uint32_t start_micros = 0;
	uint32_t dt_micros=0;
	int count=0;

	PRINTF("Start arm_fft_func\r\n");
	for (;;) {  //loop forever
		for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
			N_FFT = ALL_N_FFT[I_N_FFT];
			//PRINTF("Starting FFT %i\r\n", N_FFT);

			//initialize the FFT function
			ARM_FFT_INIT_FUNC(&cfft_inst, N_FFT, ifftFlag, doBitReverse);

			//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
			start_micros = micros();
			count = 0;
			while (count < N_FFT_LOOP) {
				count++;

				// prepare the data
				for (int j=0; j < 2*N_FFT; j += 2) {
					//first sample is real
					buffer_complex[j] = j % 8;  // 8 sample ramp

					//second sample is imaginary
					buffer_complex[j+1] = 0;  // always use a real signal
				}

				/* Process the data through the CFFT/CIFFT module */
				ARM_FFT_FUNC(&cfft_inst, buffer_complex);
			}

			// print message regarding timing
			dt_micros = micros() - start_micros;
			PRINTF("%i point ARM FFT in %4.2f usec per FFT\r\n",N_FFT,((float)dt_micros)/((float)N_FFT_LOOP));

		}
	}
	return 0;
}


#endif
