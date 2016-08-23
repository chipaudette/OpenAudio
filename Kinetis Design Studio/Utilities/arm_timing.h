#ifndef _ARM_TIMING
#define _ARM_TIMING

#define LPTMR_CLOCK_HZ CLOCK_GetFreq(kCLOCK_LpoClk)  /* Get source clock for LPTMR driver */
volatile uint32_t lptmrCounter = 0U;
void LPTMR0_IRQHandler(void) {
	LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
	lptmrCounter++;
}
float micros_per_tic;
uint16_t tics_per_loop = 30000;//must be less than 32768
void initMicros() {
	//configure the timer
	lptmr_config_t lptmrConfig;
	LPTMR_GetDefaultConfig(&lptmrConfig);
	LPTMR_Init(LPTMR0, &lptmrConfig);  /* Initialize the LPTMR */

	//now define how the timer relates to the real world
	micros_per_tic = 1000000.0/((float)LPTMR_CLOCK_HZ); //the 1000000 is because we're in microseconds
	LPTMR_SetTimerPeriod(LPTMR0, tics_per_loop);  // Set timer period...this is when it'll wrap around to zero

	//finish configuring how the processor will work with the timer
	LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);  ///it'll call its interupt when it hits wraps around, per the setting above
	EnableIRQ(LPTMR0_IRQn);  /* Enable at the NVIC */
	LPTMR_StartTimer(LPTMR0);   /* Start counting */
}
uint32_t micros() {	return (uint32_t)((uint32_t)LPTMR_GetCurrentTimerCount(LPTMR0) + (lptmrCounter*(uint32_t)tics_per_loop))*micros_per_tic; };

#endif
