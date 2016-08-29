
#include "types.h"

//Define test parameters
#define N_TRIALS 1000


//Define FIR Filter
#define N_FIR 64
filt_t b[N_FIR];
//filt_t a[1] = {0.0};
int z_ind = -1;
int foo_z_ind = 0;
filt_t z[N_FIR];
//int z[N_FIR];

//define other parameters
long start_micros=0;
long end_micros=0;
int count;
filt_t input_val;
//int input_val;
filt_t out_val;


filt_t filter(filt_t b[], filt_t &input_val, filt_t z[], int &z_ind) { 
  //put new value into state vector
  z_ind++;  if (z_ind == N_FIR) z_ind = 0; // increment and wrap (if necessary)
  z[z_ind] = input_val; // put new value into z
    
  //apply filter
  out_val = 0.0;
  foo_z_ind = z_ind;
  for (int i=0; i<N_FIR; i++) {
    foo_z_ind -= 1;  // a little faster!
    //foo_z_ind = z_ind - i; 
    if (foo_z_ind < 0) foo_z_ind = N_FIR-1;
    //foo_z_ind = (z_ind-i) % N_FIR;  //modulo.  Slow!!!
    out_val += b[i]*z[foo_z_ind];
  }

  //return the value
  return out_val;
}

void setup() {
  //Open serial for communicating to PC
  Serial.begin(115200);
  
  //initialize filter coefficients
  for (int i = 0; i < N_FIR; i++) {
    //make a running average filter
    //b[i]=1.0/N_FIR;  //assume that they are 
    b[i] = 23.1;

    //initialize filter states to zero
    z[i] = 0.0;
  }

  Serial.print("Beginning FIR Trial (float): N_FIR = ");
  Serial.println(N_FIR);
}

void loop() {
  count = 0;
  start_micros = micros();
  while (count < N_TRIALS) {
    //pretend a new value came in
    input_val = 0.874;  //some random float value
    //input_val = 1194; //some random int value

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

  //report duration
  end_micros = micros();
  float micros_per_trial = float(end_micros - start_micros) / float(N_TRIALS);
  Serial.print("N_FIR = ");
  Serial.print(N_FIR);
  Serial.print(", After ");
  Serial.print(N_TRIALS);
  Serial.print(" Trials, One Trial Averages ");
  Serial.print(micros_per_trial);
  Serial.println(" microseconds");

}
