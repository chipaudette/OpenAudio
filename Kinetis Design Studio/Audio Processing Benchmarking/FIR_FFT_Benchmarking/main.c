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

#include "..\..\Utilities\arm_timing.h" //my own functions for micros() on ARM

//In this header file is the definition for things like DataType (int16, int32, float) and MAX_N
#include "..\..\..\Arduino\Audio Processing Benchmarking\FIR_FFT_Benchmarking\fft_fir_const.h"
//#include "fft_fir_const.h"

// Choose the operation that you want to do
#define DO_NAIVE_FIR 0
#define DO_KISS_FFT 1
#define DO_ARM_FFT 2
#define OPERATION_TO_DO DO_ARM_FFT  //change this to select which function to run


#if OPERATION_TO_DO == DO_NAIVE_FIR
#include "..\..\..\Arduino\Audio Processing Benchmarking\FIR_FFT_Benchmarking\do_naive_fir.h"
char *alg_name = "NAIVE FIR";
#define N_TRIALS 10000   // how many times to repeat the operation
#endif

#if (OPERATION_TO_DO == DO_KISS_FFT)
//in Kinetis Design Studio, be sure to increase the HEAP size!
//Under settings/MK66FN2M0xxx18_flash.ld make this change:
//    HEAP_SIZE  = DEFINED(__heap_size__)  ? __heap_size__  : 0x4400; /* WEA 2016-08-20 */
#include "..\..\..\Arduino\Audio Processing Benchmarking\FIR_FFT_Benchmarking\do_kiss_fft.h"
char *alg_name = "KISS FFT";
#define N_TRIALS 1000   // how many times to repeat the operation
#endif

#if (OPERATION_TO_DO == DO_ARM_FFT)
#include "..\..\..\Arduino\Audio Processing Benchmarking\FIR_FFT_Benchmarking\do_arm_fft.h"
char *alg_name = "ARM FFT";
#define N_TRIALS 1000   // how many times to repeat the operation
#endif


#ifdef IS_ARDUINO_UNO
int freeRam(void) {
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#else
int freeRam(void) { return -1; }
#endif

int main(void) {
	int N;
	int32_t dt_micros=0;

	BOARD_InitPins();
	//BOARD_BootClockRUN();   //120 MHz
	BOARD_BootClockHSRUN();   //180MHz
	BOARD_InitDebugConsole();

	initMicros();

	PRINTF("FIR/FFT Benchmarking...\r\n");
	PRINTF("  : System Clock = %i Hz\r\n",CLOCK_GetFreq(kCLOCK_CoreSysClk));
	//PRINTF("  : LPTMR Source Clock = %i Hz\r\n", LPTMR_CLOCK_HZ);
	//PRINTF("  : micros per tick = %4.1f\r\n", millis_per_tic*1000);

	//loop forever
	N = MAX_N;
	int N_LOOP = 0;
	for (;;) {
		if (N_LOOP < 4) {

			//do the processing
			#if (OPERATION_TO_DO == DO_NAIVE_FIR)
			  dt_micros = naive_fir_func(N,N_TRIALS);
			#elif (OPERATION_TO_DO == DO_KISS_FFT)
			  dt_micros = kiss_fft_func(N,N_TRIALS);
			#elif (OPERATION_TO_DO == DO_ARM_FFT)
			  dt_micros = arm_fft_func(N,N_TRIALS);
			#endif

			//report the timing
			PRINTF("%s: N = %i\tin %6.1f usec per operation\r\n",alg_name,N,((float)dt_micros)/((float)N_TRIALS));

			//prepare for next step
			N = N/2;
			if (N < 8) {
				N = MAX_N;
				N_LOOP++;
			}

		}
	}
	return 0;
}
