

//include "arm_math.h"  //to get int16_t and int32_t 
#include "mytypes.h"

//Add streaming-like output http://playground.arduino.cc/Main/StreamingOutput
template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}

const int ledPin = 13;  //Teensy 3.0+ is on pin 13

#define MAX_NFFT 1024
#define N_ALL_FFT 1
int ALL_N_FFT[N_ALL_FFT] = {MAX_NFFT};
int ALL_LOG2_N_FFT[N_ALL_FFT] = {10};
int N_FFT = MAX_NFFT;
int LOG2_N_FFT = 10;
int real[MAX_NFFT];
int imag[MAX_NFFT];


void setup() {
  //start the serial
  Serial.begin(115200);

  //delay to allow the serial to really get going
  delay(2000);

  //prepare for blinky light
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

}

#define N_LOOP 200
int count = 0;
long start_micros = 0;
long accumulated_micros = 0;
int led_status = LOW;
int ret_val = 0;
void loop() {

  for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
    N_FFT = ALL_N_FFT[I_N_FFT];
    LOG2_N_FFT =  ALL_LOG2_N_FFT[I_N_FFT];

    //Serial << "Allocating memory for N = " << N_FFT << '\n';
    int is_ifft = 0;
   
    accumulated_micros = 0;
    count = 0;

    //Serial << "Beginning N_LOOP = " << N_LOOP << '\n';
    while (count < N_LOOP) {
      led_status = HIGH;
      digitalWrite(ledPin, led_status);
      count++;

      //prepare input data
      for (int i = 0; i < N_FFT; i++) {
        real[i] = (i % 8); //real
        //real[i] = 1000*cos(2*3.1415926535 * ((float)i/10.0));// real
        imag[i] = 0; //imaginary
      }

      //start timer
      start_micros = micros();

      //do fft
      ret_val = fix_fft(real,imag,LOG2_N_FFT,is_ifft);
      //Serial << "ret_val = " << ret_val << '\n';

      //finish timer
      accumulated_micros += (micros() - start_micros);
      //Serial << "FFT " << count << " complete.\n";
    }

    if (accumulated_micros != 0) {
      //print the timing results
      Serial.print("N_FFT = ");
      Serial.print(N_FFT);
      Serial.print(", micros per FFT = ");
      Serial.println( ((float)accumulated_micros) / ((float)N_LOOP));
    }

  }

}

