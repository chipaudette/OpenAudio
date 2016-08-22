
#ifndef _DO_NAIVE_FIR
#define _DO_NAIVE_FIR


// /////////////////////// DEFINE FIR PARAMETERS
//Define test parameters
#define N_TRIALS 10000

//Define the different-length FIR Filters
#define N_FIR_MAX 1024
#define N_all_N_FIR (9)
int all_N_FIR[N_all_N_FIR] = {N_FIR_MAX/256, N_FIR_MAX/128, N_FIR_MAX/64, N_FIR_MAX/32, N_FIR_MAX/16, N_FIR_MAX/8, N_FIR_MAX/4, N_FIR_MAX/2, N_FIR_MAX};

// Define data type
//typedef double filt_t;
//typedef float filt_t;
//typedef long filt_t;
typedef int filt_t;

// define filter size

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
	uint32_t start_micros = 0;
	uint32_t dt_micros=0;
	int count=0;

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
		for (int I_loop = 0; I_loop < 1; I_loop++) { //run multiple times?  or not.
			count = 0;
			z_ind=0;

			start_micros = micros();
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

			// print message regarding timing
			dt_micros = micros() - start_micros;
			PRINTF("%i point NAIVE FIR in %4.1f usec per FIR\r\n",N_FIR,((float)dt_micros)/((float)N_TRIALS));
		}
	}

	return 0;
}

#endif
