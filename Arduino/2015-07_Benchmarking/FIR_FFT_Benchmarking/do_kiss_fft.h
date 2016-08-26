
#ifndef _DO_KISS_FFT
#define _DO_KISS_FFT

// parameters for stepping through different FFT lengths
//#define MAX_N_FFT 2048
//#define N_ALL_FFT 7
//int ALL_N_FFT[N_ALL_FFT] = {MAX_N_FFT/64, MAX_N_FFT/32, MAX_N_FFT/16, MAX_N_FFT/8, MAX_N_FFT/4, MAX_N_FFT/2, MAX_N_FFT};
//#define N_FFT_LOOP 1000

#include "FFT_FIR_Const.h"
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"

//int len_mem = sizeof(kiss_fft_state_t) + sizeof(kiss_fft_cpx)*(MAX_N-1); /* twiddle factors*/
//#define MAX_N 64
const int const_len_mem = sizeof(kiss_fft_state_t) + sizeof(kiss_fft_cpx)*(MAX_N-1); /* twiddle factors*/
size_t len_mem = (size_t)const_len_mem;
char fft_twiddle[const_len_mem];  //holds the twiddle factors
kiss_fft_cpx in_buffer[MAX_N];
kiss_fft_cpx out_buffer[MAX_N];
kiss_fft_cfg my_fft_cfg;

float out_val;
uint32_t start_micros = 0;
int is_ifft=0; //change this to 1 to run an IFFT instead of an FFT
uint32_t kiss_fft_func(const int N_FFT, const int N_LOOP) {
  if (N_FFT > MAX_N) return 0; // don't ask for N that is too big

	//Configure the memory (and FFT twiddle factors) for this trial
	//my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,NULL, NULL);
  my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,fft_twiddle,&len_mem);
  if ((len_mem == 0) || (len_mem == NULL) || (my_fft_cfg == NULL)) return 0; //this means it couldn't configure the memroy

	//PRINTF("STARTING N_FFT_LOOP = %i\r\n", N_FFT_LOOP);
	start_micros = micros();
	for (int count=0; count < N_LOOP; count++) {

		//prepare the fake input data
		for (int i = 0; i < N_FFT; i++) {
			//in_buffer[i].r = (kiss_fft_scalar)(i % 8); //real
      in_buffer[i].r = (kiss_fft_scalar)(i); //real
			in_buffer[i].i = (kiss_fft_scalar)0; //imaginary
		}

		//do fft
		kiss_fft(my_fft_cfg,in_buffer,out_buffer);
	}

  // return the time it took to do this run
	return micros() - start_micros;
};


#endif
