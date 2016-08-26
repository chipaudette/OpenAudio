
#ifndef _DO_NAIVE_FIR
#define _DO_NAIVE_FIR

#include "FFT_FIR_Const.h"

// Define data type
//typedef double filt_t;
//typedef float filt_t;
//typedef long filt_t;
//typedef int filt_t;


filt_t out_val;  //must be outside the function to prevent optimizer from removing the meat of the function
filt_t b[MAX_N],z[MAX_N];
uint32_t naive_fir_func(const int N_FIR, const int N_LOOP) {
	uint32_t start_micros = 0;
	uint32_t dt_micros=0;
	filt_t input_val;

	// define filter size
	int z_ind = -1;
	int foo_z_ind = 0;

	//initialize filter coefficients
	for (int i = 0; i < N_FIR; i++) {
		//define the filter coefficients
		b[i] = 23.1+(float)i;

		//initialize filter states to zero
		z[i] = 0.0+(float)i;
	}

	//start doing the trials
	start_micros = micros();
	for (int count = 0; count < N_LOOP; count++) { //run multiple times?  or not.
		z_ind=0;

		//pretend a new value came in
		//input_val = 1194.23; //some unusual value
		input_val = (filt_t)1194.23+(filt_t)count; //adding the (float)count slows things down quite a bit!

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
	}

	// assess the timing
	dt_micros = micros() - start_micros;
	//PRINTF("%i point NAIVE FIR in %4.2f usec per FIR\r\n",N_FIR,((float)dt_micros)/((float)N_TRIALS));

	return dt_micros;
}

#endif
