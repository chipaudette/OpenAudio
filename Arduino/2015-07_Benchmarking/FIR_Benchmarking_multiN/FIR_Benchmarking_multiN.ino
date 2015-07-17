
#include "types.h"

//Define test parameters...set to 1000 for double and float, 8000 for long and int
#define N_TRIALS 8000

//Define FIR Filters
#define N_all_N_FIR (8)
int all_N_FIR[N_all_N_FIR];

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

void setup() {
  //Open serial for communicating to PC
  Serial.begin(115200);
  
  for (int i = 0; i 
  < N_all_N_FIR; i++) {
    all_N_FIR[i]= pow(2,i+2);
    
  }
  
  
  //initialize filter coefficients
  for (int i = 0; i < N_FIR_MAX; i++) {
    //make a running average filter
    //b[i]=1.0/N_FIR;  //assume that they are 
    b[i] = 23.1;
    

    //initialize filter states to zero
    z[i] = 0.0;
  }

  Serial.print("Beginning FIR Trial (float): N_FIR_MAX = ");
  Serial.println(N_FIR_MAX);
}

int I_N_FIR = -1;
int N_FIR;
void loop() {
  
  I_N_FIR = (I_N_FIR + 1) % N_all_N_FIR;
  N_FIR = all_N_FIR[I_N_FIR];
    
  for (int I_loop = 0; I_loop < 5; I_loop++) {
        
    count = 0;
    z_ind=0;
    
    
    start_micros = micros();
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
 
}
