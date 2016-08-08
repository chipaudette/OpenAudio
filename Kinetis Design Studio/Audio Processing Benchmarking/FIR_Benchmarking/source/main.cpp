
//include "fsl_device_registers.h"  //WEA added this
#include "fsl_debug_console.h" //WEA added this
#include "board.h"

#include "fsl_lptmr.h"
#include "fsl_gpio.h"

#include "pin_mux.h"
#include "clock_config.h"
#include <math.h>

// For timer
#define LPTMR_LED_HANDLER LPTMR0_IRQHandler
#define LPTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_LpoClk)  /* Get source clock for LPTMR driver */
volatile uint32_t lptmrCounter = 0U;
void LPTMR_LED_HANDLER(void)
{
    LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
    lptmrCounter++;
}

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

//define other parameters
long start_micros=0;
long end_micros=0;
int count=0;

filt_t input_val;
filt_t out_val;

int I_N_FIR = -1;
int N_FIR;
int main(void) {


  /* Init board hardware. */
  BOARD_InitPins();
  BOARD_BootClockRUN();
  BOARD_InitDebugConsole();

  /* Configure LPTMR */
  lptmr_config_t lptmrConfig;
  LPTMR_GetDefaultConfig(&lptmrConfig);
  LPTMR_Init(LPTMR0, &lptmrConfig);  /* Initialize the LPTMR */
  LPTMR_SetTimerPeriod(LPTMR0, USEC_TO_COUNT(1000000U, LPTMR_SOURCE_CLOCK));  /* Set timer period */
  LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);  /* Enable timer interrupt */
  EnableIRQ(LPTMR0_IRQn);  /* Enable at the NVIC */

  PRINTF("FIR Benchmarking...\r\n");

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
  LPTMR_StartTimer(LPTMR0);   /* Start counting */
  for(;;) { /* Infinite loop to avoid leaving the main function */

	  I_N_FIR = (I_N_FIR + 1) % N_all_N_FIR;
	  N_FIR = all_N_FIR[I_N_FIR];

	  //do each filter five times
	  for (int I_loop = 0; I_loop < 5; I_loop++) {

		  count = 0;
		  z_ind=0;


		  start_micros = 1000*lptmrCounter;
		  while (count < N_TRIALS) {
			  //pretend a new value came in
			  //input_val = 0.874;  //some random float value
			  input_val = 1194.23; //some random int value

			  //filter the value
			  #if 0
			  	  out_val = filter(b,input_val,z,z_ind);
			  #else
				  //put new value into state vector
				  z_ind++;  if (z_ind == N_FIR) z_ind = 0; // increment and wrap (if necessary)
				  z[z_ind] = input_val; // put new value into z

				  //apply filter
				  out_val = 0.0;
				  foo_z_ind = z_ind; //not needed for modulo
				  for (int i=0; i<N_FIR; i++) {
					  foo_z_ind -= 1; if (foo_z_ind < 0) foo_z_ind = N_FIR-1; // a little faster!
					  //foo_z_ind = z_ind - i; if (foo_z_ind < 0) foo_z_ind = N_FIR-1;
					  //foo_z_ind = (z_ind-i) % N_FIR;  //modulo.  Slow on Uno.  Not bad on M0 Pro.
					  out_val += b[i]*z[foo_z_ind];
				  }
			  #endif

			  //increment loop counter
			  count++;
		  }
		  end_micros = 1000*lptmrCounter;

		  // print message saying during
		  PRINTF("%i-point FIR in %i usec per FIR\r\n",N_FIR,(end_micros-start_micros)/N_TRIALS);
	  }
  }
}
