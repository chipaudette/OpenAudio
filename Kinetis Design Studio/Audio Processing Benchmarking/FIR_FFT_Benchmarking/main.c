/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "fsl_lptmr.h"
#include "fsl_gpio.h"
#include <math.h>


#define DO_FIR 0
#define DO_FFT 1
#define DO_FFT_CMSIS 2
#define OPERATION_TO_DO DO_FFT_CMSIS  //comment this to do FIR

// For millis()
#define LPTMR_CLOCK_HZ CLOCK_GetFreq(kCLOCK_LpoClk)  /* Get source clock for LPTMR driver */
volatile uint32_t lptmrCounter = 0U;
void LPTMR0_IRQHandler(void) {LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);lptmrCounter++;}
float millis_per_tic;
uint16_t tics_per_loop = 30000;//must be less than 32768
void initMillis() {
	//configure the timer
	lptmr_config_t lptmrConfig;
	LPTMR_GetDefaultConfig(&lptmrConfig);
	LPTMR_Init(LPTMR0, &lptmrConfig);  /* Initialize the LPTMR */

	//now define how the timer relates to the real world
	millis_per_tic = 1000.0/((float)LPTMR_CLOCK_HZ); //the 1000 is because we're in milliseconds
	LPTMR_SetTimerPeriod(LPTMR0, tics_per_loop);  // Set timer period...this is when it'll wrap around to zero

	//finish configuring how the processor will work with the timer
	LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);  ///it'll call its interupt when it hits wraps around, per the setting above
	EnableIRQ(LPTMR0_IRQn);  /* Enable at the NVIC */
	LPTMR_StartTimer(LPTMR0);   /* Start counting */
}
uint32_t millis() {	return (uint32_t)((uint32_t)LPTMR_GetCurrentTimerCount(LPTMR0) + (lptmrCounter*(uint32_t)tics_per_loop))*millis_per_tic; };

//define timing parameters
uint32_t start_tics = 0;
uint32_t end_tics = 0;
int count=0;

#if OPERATION_TO_DO == DO_FIR

// /////////////////////// DEFINE FIR PARAMETERS
//Define test parameters
#define N_TRIALS 10000

//Define the different-length FIR Filters
#define N_all_N_FIR (8)
int all_N_FIR[N_all_N_FIR];

// Define data type
//typedef double filt_t;
//typedef float filt_t;
//typedef long filt_t;
typedef int filt_t;

// define filter size
#define N_FIR_MAX 512
filt_t b[N_FIR_MAX];
int z_ind = -1;
int foo_z_ind = 0;
filt_t z[N_FIR_MAX];

filt_t input_val;
filt_t out_val;

int I_N_FIR = -1;
int N_FIR;
////////////// END FIR Parameters

int naive_fir_func(void) {
	/* Initialize FIR settings */
	for (int i = 0; i  < N_all_N_FIR; i++) {
		all_N_FIR[i]= pow(2,i+2);
	}

	//initialize filter coefficients
	for (int i = 0; i < N_FIR_MAX; i++) {
		//make a running average filter
		//b[i]=1.0/N_FIR;  //assume that they are
		b[i] = 23.1+(float)i;

		//initialize filter states to zero
		z[i] = 0.0+(float)i;
	}

	// begin the main loop
	for(;;) { /* Infinite loop to avoid leaving the main function */

		I_N_FIR = (I_N_FIR + 1) % N_all_N_FIR;
		N_FIR = all_N_FIR[I_N_FIR];

		//do each filter five times
		for (int I_loop = 0; I_loop < 5; I_loop++) {

			count = 0;
			z_ind=0;


			//lptmrCounter = 0U;
			//start_micros = micros_per_interrupt*lptmrCounter;
			//start_millis = millis();
			start_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;
			while (count < N_TRIALS) {
				//pretend a new value came in
				input_val = 1194.23; //some unusual value
				//input_val = 1194.23+(float)count; //adding the (float)count slows things down quite a bit!

				//put new value into state vector
				z_ind++;  if (z_ind == N_FIR) z_ind = 0; // increment and wrap (if necessary)
				z[z_ind] = input_val; // put new value into z

				//apply filter
				out_val = 0.0; //initialize the output value
				foo_z_ind = z_ind; //not needed for modulo
				for (int i=0; i<N_FIR; i++) { //loop over each filter tap
					foo_z_ind -= 1; if (foo_z_ind < 0) foo_z_ind = N_FIR-1; // a little faster!
					//foo_z_ind = z_ind - i; if (foo_z_ind < 0) foo_z_ind = N_FIR-1;
					//foo_z_ind = (z_ind-i) % N_FIR;  //modulo.  Slow on Uno.  Not bad on M0 Pro.
					out_val += b[i]*z[foo_z_ind];
				}

				//increment loop counter
				count++;
			}
			//end_micros = micros_per_interrupt*lptmrCounter;
			//end_millis = millis();
			end_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;

			// print message saying during
			//float dt_millis = ((end_tics-start_tics) + lptmrCounter*tics_per_loop)*millis_per_tic ;
			float dt_millis = (end_tics-start_tics)*millis_per_tic;
			PRINTF("%i point FIR in %4.2f usec per FIR\r\n",N_FIR,1000.0*(dt_millis/((float)N_TRIALS)));
		}
	}

	return 0;
}
#else
int naive_fir_func(void);
#endif

// /////////////////////////// DEFINE FFT PARAMETERS
#if ((OPERATION_TO_DO == DO_FFT) || (OPERATION_TO_DO == DO_FFT_CMSIS))
#define MAX_N_FFT 2048
#define N_ALL_FFT 7
int ALL_N_FFT[N_ALL_FFT] = {32, 64, 128, 256, 512, 1024, 2048};
int N_FFT;
#define N_FFT_LOOP 1000
#endif

#if (OPERATION_TO_DO == DO_FFT)
#include "fftsettings.h"
#include "kiss_fft.h"
kiss_fft_cpx in_buffer[MAX_N_FFT];
kiss_fft_cpx out_buffer[MAX_N_FFT];
kiss_fft_cfg my_fft_cfg;
int kiss_fft_func(void) {
	float dt_millis=0;
	int is_ifft=0;

	PRINTF("Start kiss_fft_func\r\n");
	for (;;) {  //loop forever

		for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
			N_FFT = ALL_N_FFT[I_N_FFT];
			//PRINTF("Starting FFT %i\r\n", N_FFT);

			//Serial << "Allocating memory for N = " << N_FFT << '\n';
			is_ifft = 0;
			if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
			my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,NULL, NULL);

			count = 0;

			start_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;
			//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
			while (count < N_FFT_LOOP) {
				count++;

				//prepare input data
				for (int i = 0; i < N_FFT; i++) {
					in_buffer[i].r = (kiss_fft_scalar)(i % 8); //real
					in_buffer[i].i = (kiss_fft_scalar)0; //imaginary
				}

				//do fft
				//PRINTF("Starting FFT %i\r\n", count);
				kiss_fft(my_fft_cfg,in_buffer,out_buffer);
				//PRINTF("FFT %i completer\r\n", count++);

			}

			//end_millis = millis();
			end_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;

			// print message saying during
			//float dt_millis = ((end_tics-start_tics) + lptmrCounter*tics_per_loop)*millis_per_tic ;
			dt_millis = (end_tics-start_tics)*millis_per_tic;
			PRINTF("%i point FFT in %4.2f usec per FFT\r\n",N_FFT,1000.0*(dt_millis/((float)N_FFT_LOOP)));

		}
	}
	return 0;
};
#else
int kiss_fft_func(void);
#endif

#if (OPERATION_TO_DO == DO_FFT_CMSIS)
#define ARM_MATH_CM4
#include "arm_math.h"
#include "arm_const_structs.h" //from CMSIS example.  Is it needed?

uint8_t ifftFlag = 0;
uint8_t doBitReverse = 1;

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

// //arm_cfft_f32(&arm_cfft_sR_f32_len1024, testInput_f32_10khz, ifftFlag, doBitReverse);
// //arm_cfft_radix2_q15(fft_instance,input_data)
// //arm_cfft_q31(fft_instance,input_complex_Q31,ifftFlag,bitReverseFlag);
// //if (window) apply_window_to_fft_buffer(buffer, window);
// //arm_cfft_radix4_q15(&fft_inst, buffer_complex_Q15);
//arm_cfft_radix2_q15(&cfft_inst, buffer_complex);
//arm_cfft_radix2_q31(&cfft_inst, buffer_complex);
//arm_cfft_radix4_q15(&cfft_inst, buffer_complex);
//arm_cfft_radix4_q31(&cfft_inst, buffer_complex);
//arm_cfft_radix2_f32(&cfft_inst, buffer_complex);
//arm_cfft_radix4_f32(&cfft_inst, buffer_complex);

int arm_fft_func(void) {
	float dt_millis=0;

		PRINTF("Start arm_fft_func\r\n");
		for (;;) {  //loop forever

			for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
				N_FFT = ALL_N_FFT[I_N_FFT];
				//PRINTF("Starting FFT %i\r\n", N_FFT);


				//initialize the FFT function
				ARM_FFT_INIT_FUNC(&cfft_inst, N_FFT, ifftFlag, doBitReverse);

				count = 0;

				start_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;
				//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
				while (count < N_FFT_LOOP) {
					count++;

			        // prepare the data
			        //for (int j=0; j < N_FFT; j++) buffer_real[j]=j%8;// 8 sample ramp
			        for (int j=0; j < 2*N_FFT; j += 2) {
			          //first sample is real
			          buffer_complex[j] = j % 8;  // 8 sample ramp

			          //second sample is imaginary
			          buffer_complex[j+1] = 0;  // always use a real signal
			        }

			        /* Process the data through the CFFT/CIFFT module */
			        ARM_FFT_FUNC(&cfft_inst, buffer_complex);

				}

				//end_millis = millis();
				end_tics = LPTMR_GetCurrentTimerCount(LPTMR0) + lptmrCounter*tics_per_loop;

				// print message saying during
				//float dt_millis = ((end_tics-start_tics) + lptmrCounter*tics_per_loop)*millis_per_tic ;
				dt_millis = (end_tics-start_tics)*millis_per_tic;
				PRINTF("%i point FFT in %4.2f usec per FFT\r\n",N_FFT,1000.0*(dt_millis/((float)N_FFT_LOOP)));

			}
		}
		return 0;
}

#else
	int arm_fft_func(void);
#endif

int main(void) {

	BOARD_InitPins();
	//BOARD_BootClockRUN();   //120 MHz
	BOARD_BootClockHSRUN(); //180MHz
	BOARD_InitDebugConsole();

	initMillis();

	PRINTF("FIR/FFT Benchmarking...\r\n");
	PRINTF("  : System Clock = %i Hz\r\n",CLOCK_GetFreq(kCLOCK_CoreSysClk));
	PRINTF("  : LPTMR Source Clock = %i Hz\r\n", LPTMR_CLOCK_HZ);
	//PRINTF("  : ticks per interrupt = %i\r\n", tics_per_interrupt);
	PRINTF("  : micros per tick = %4.1f\r\n", millis_per_tic*1000);

#if OPERATION_TO_DO == DO_FIR
	naive_fir_func();
#elif OPERATION_TO_DO == DO_FFT
	kiss_fft_func();
#elif OPERATION_TO_DO == DO_FFT_CMSIS
	arm_fft_func();
#endif

	return 0;
}