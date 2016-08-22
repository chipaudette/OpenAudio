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

#define DO_NAIVE_FIR 0
#define DO_KISS_FFT 1
#define DO_ARM_FFT 2
#define OPERATION_TO_DO DO_NAIVE_FIR  //change this to select which function to run

// For millis()
#define LPTMR_CLOCK_HZ CLOCK_GetFreq(kCLOCK_LpoClk)  /* Get source clock for LPTMR driver */
volatile uint32_t lptmrCounter = 0U;
void LPTMR0_IRQHandler(void) {
	LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
	lptmrCounter++;
}
float millis_per_tic;
float micros_per_tic;
uint16_t tics_per_loop = 30000;//must be less than 32768
void initMicros() {
	//configure the timer
	lptmr_config_t lptmrConfig;
	LPTMR_GetDefaultConfig(&lptmrConfig);
	LPTMR_Init(LPTMR0, &lptmrConfig);  /* Initialize the LPTMR */

	//now define how the timer relates to the real world
	//millis_per_tic = 1000.0/((float)LPTMR_CLOCK_HZ); //the 1000 is because we're in milliseconds
	micros_per_tic = 1000000.0/((float)LPTMR_CLOCK_HZ); //the 1000000 is because we're in microseconds
	LPTMR_SetTimerPeriod(LPTMR0, tics_per_loop);  // Set timer period...this is when it'll wrap around to zero

	//finish configuring how the processor will work with the timer
	LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);  ///it'll call its interupt when it hits wraps around, per the setting above
	EnableIRQ(LPTMR0_IRQn);  /* Enable at the NVIC */
	LPTMR_StartTimer(LPTMR0);   /* Start counting */
}
uint32_t micros() {	return (uint32_t)((uint32_t)LPTMR_GetCurrentTimerCount(LPTMR0) + (lptmrCounter*(uint32_t)tics_per_loop))*micros_per_tic; };


#if OPERATION_TO_DO == DO_NAIVE_FIR
#include "do_naive_fir.h"
#endif

#if (OPERATION_TO_DO == DO_KISS_FFT)
#include "do_kiss_fft.h"
#endif

#if (OPERATION_TO_DO == DO_ARM_FFT)
#include "do_arm_fft.h"
#endif

int main(void) {

	BOARD_InitPins();
	//BOARD_BootClockRUN();   //120 MHz
	BOARD_BootClockHSRUN(); //180MHz
	BOARD_InitDebugConsole();

	initMicros();

	PRINTF("FIR/FFT Benchmarking...\r\n");
	PRINTF("  : System Clock = %i Hz\r\n",CLOCK_GetFreq(kCLOCK_CoreSysClk));
	PRINTF("  : LPTMR Source Clock = %i Hz\r\n", LPTMR_CLOCK_HZ);
	//PRINTF("  : ticks per interrupt = %i\r\n", tics_per_interrupt);
	PRINTF("  : micros per tick = %4.1f\r\n", millis_per_tic*1000);

#if (OPERATION_TO_DO == DO_NAIVE_FIR)
	naive_fir_func();
#elif (OPERATION_TO_DO == DO_KISS_FFT)
	kiss_fft_func();
#elif (OPERATION_TO_DO == DO_ARM_FFT)
	arm_fft_func();
#endif

	return 0;
}
