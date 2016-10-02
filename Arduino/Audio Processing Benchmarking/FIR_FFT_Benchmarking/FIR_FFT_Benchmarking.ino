/*
This runs FIR and FFT computation routines.  My purpose is to see how fast different
platforms are when doing this kind of math.

Created: OpenAudio (Chip Audette)
       : August 2016

 */

// Choose the operation that you want to do
#define DO_NAIVE_FIR 0
#define DO_KISS_FFT 1
#define DO_ARM_FFT 2
#define OPERATION_TO_DO DO_NAIVE_FIR  //change this to select which function to run


//In this header file is the definition for things like DataType (int16, int32, float) and MAX_N
#include "FFT_FIR_Const.h"

#if OPERATION_TO_DO == DO_NAIVE_FIR
#include "do_naive_fir.h"
char *alg_name = "NAIVE FIR";
#define N_TRIALS 500   // how many times to repeat the operation
#endif

#if (OPERATION_TO_DO == DO_KISS_FFT)
#include "do_kiss_fft.h"
char *alg_name = "KISS FFT";
#define N_TRIALS 5   // how many times to repeat the operation
#endif

#if (OPERATION_TO_DO == DO_ARM_FFT)
  #include "do_arm_fft.h"
  char *alg_name = "ARM FFT";
  #define N_TRIALS 50   // how many times to repeat the operation
#endif

// parameters for stepping through different FFT lengths
#if (MAX_N <= 128)
  #define LEN_ALL_N 4
  int ALL_N[LEN_ALL_N] = {MAX_N, MAX_N/2, MAX_N/4, MAX_N/8};
#else
  #define LEN_ALL_N 8
  int ALL_N[LEN_ALL_N] = {MAX_N, MAX_N/2, MAX_N/4, MAX_N/8, MAX_N/16, MAX_N/32, MAX_N/64, MAX_N/128};
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

void setup() {
  Serial.begin(115200);  
  delay(1000);
  
  Serial.println(F("FIR/FFT Benchmarking..."));delay(500);
  //PRINTF("  : System Clock = %i Hz\r\n",CLOCK_GetFreq(kCLOCK_CoreSysClk));
  //PRINTF("  : LPTMR Source Clock = %i Hz\r\n", LPTMR_CLOCK_HZ);
  //PRINTF("  : micros per tick = %4.1f\r\n", millis_per_tic*1000);
}

int I_loop=0;
int N;
uint32_t dt_micros;
void loop() {
 
  if (I_loop < 3) {
     I_loop++;

		//step trough each N
		for (int I_N = 0; I_N < LEN_ALL_N;  I_N++) {
			N = ALL_N[I_N];
      if (N >= 4) {
        
  			//do the processing
  			#if (OPERATION_TO_DO == DO_NAIVE_FIR)
  				dt_micros = naive_fir_func(N,N_TRIALS);
  			#elif (OPERATION_TO_DO == DO_KISS_FFT)
  				dt_micros = kiss_fft_func(N,N_TRIALS);
  			#elif (OPERATION_TO_DO == DO_ARM_FFT)
  				dt_micros = arm_fft_func(N,N_TRIALS);
  			#endif
  
  			//report the timing
        //PRINTF("%s: N = %i\tin %6.1f usec per operation\r\n",alg_name,N,((float)dt_micros)/((float)N_TRIALS));
        Serial.print(alg_name); Serial.print(F(": N = ")); Serial.print(N); Serial.print(F(" in ")); 
        if (dt_micros < 1) {
          Serial.print(F(" *** NOT ENOUGH TEMP MEMORY *** "));
        } else {
          Serial.print(((float)dt_micros)/((float)N_TRIALS));
          Serial.print(F(" usec per operation"));  
        }
        #ifdef IS_ARDUINO_UNO
          Serial.print(F(", Free RAM = ")); Serial.print(freeRam());
        #endif
        Serial.println();
      }
		}
	}
}


