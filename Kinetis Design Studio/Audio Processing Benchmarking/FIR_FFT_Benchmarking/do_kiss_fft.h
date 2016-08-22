
#ifndef _DO_KISS_FFT
#define _DO_KISS_FFT

// parameters for stepping through different FFT lengths
#define MAX_N_FFT 2048
#define N_ALL_FFT 7
int ALL_N_FFT[N_ALL_FFT] = {MAX_N_FFT/64, MAX_N_FFT/32, MAX_N_FFT/16, MAX_N_FFT/8, MAX_N_FFT/4, MAX_N_FFT/2, MAX_N_FFT};
#define N_FFT_LOOP 1000


#include "kiss_fft.h"
kiss_fft_cpx in_buffer[MAX_N_FFT];
kiss_fft_cpx out_buffer[MAX_N_FFT];
kiss_fft_cfg my_fft_cfg;
int kiss_fft_func(void) {
	int N_FFT;
	uint32_t start_micros = 0;
	uint32_t dt_micros=0;
	int count=0;
	int is_ifft=0; //change this to 1 to run an IFFT instead of an FFT
	PRINTF("Start kiss_fft_func\r\n");

	// KISS_FFT uses malloc() to dynamically allocate memroy.  This can cause memory
	// fragmentation which can quickly make a microcontroller run out of RAM.  To
	// reduce the chances of this, let's grab enough memory for biggest possible run.
	if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
	my_fft_cfg = kiss_fft_alloc(MAX_N_FFT,is_ifft,NULL, NULL);

	//start looping to do all of the trials
	for (;;) {  //loop forever

		for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
			N_FFT = ALL_N_FFT[I_N_FFT];
			//PRINTF("Starting FFT %i\r\n", N_FFT);

			//Allocate memory for this trial
			if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
			my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,NULL, NULL);


			//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
			start_micros = micros();
			for (count=0; count < N_FFT_LOOP; count++) {

				//prepare input data
				for (int i = 0; i < N_FFT; i++) {
					in_buffer[i].r = (kiss_fft_scalar)(i % 8); //real
					in_buffer[i].i = (kiss_fft_scalar)0; //imaginary
				}

				//do fft
				kiss_fft(my_fft_cfg,in_buffer,out_buffer);
			}

			// print message regarding timing
			dt_micros = micros() - start_micros;
			PRINTF("%i point KISS FFT in %4.1f usec per FFT\r\n",N_FFT,((float)dt_micros)/((float)N_FFT_LOOP));
		}
	}
	return 0;
};

#endif
