
#ifndef _DO_KISS_FFT
#define _DO_KISS_FFT

// parameters for stepping through different FFT lengths
//#define MAX_N_FFT 2048
//#define N_ALL_FFT 7
//int ALL_N_FFT[N_ALL_FFT] = {MAX_N_FFT/64, MAX_N_FFT/32, MAX_N_FFT/16, MAX_N_FFT/8, MAX_N_FFT/4, MAX_N_FFT/2, MAX_N_FFT};
//#define N_FFT_LOOP 1000


#include "kiss_fft.h"
float out_val;
int32_t kiss_fft_func(const int N_FFT, const int N_LOOP) {
	uint32_t start_micros = 0;
	uint32_t dt_micros=0;
	int is_ifft=0; //change this to 1 to run an IFFT instead of an FFT

	kiss_fft_cpx in_buffer[N_FFT];
	kiss_fft_cpx out_buffer[N_FFT];
	kiss_fft_cfg my_fft_cfg;

	// KISS_FFT uses malloc() to dynamically allocate memroy.  This can cause memory
	// fragmentation which can quickly make a microcontroller run out of RAM.  To
	// reduce the chances of this, let's grab enough memory for biggest possible run.
	//if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
	//my_fft_cfg = kiss_fft_alloc(MAX_N_FFT,is_ifft,NULL, NULL);


	//Allocate memory for this trial
	//if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
	my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,NULL, NULL);


	//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
	start_micros = micros();
	for (int count=0; count < N_LOOP; count++) {

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
	//PRINTF("%i point KISS FFT in %4.1f usec per FFT\r\n",N_FFT,((float)dt_micros)/((float)N_FFT_LOOP));

	//dummy to prevent optimizer from over-optimizing
	//out_val = out_buffer[2];  //choose an index at random

	//de-allocate the memory
	KISS_FFT_FREE(my_fft_cfg);

	return dt_micros;
};

#endif
