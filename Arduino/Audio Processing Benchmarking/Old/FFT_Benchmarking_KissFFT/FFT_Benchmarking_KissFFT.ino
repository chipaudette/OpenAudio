

//add KISS_FFT library: http://sourceforge.net/projects/kissfft/?source=typ_redirect
#include "arm_math.h"  //to get int16_t and int32_t 
#include "mytypes.h"
#include "kiss_fft.h"

//Add streaming-like output http://playground.arduino.cc/Main/StreamingOutput
template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}

const int ledPin = 13;  //Teensy 3.0+ is on pin 13

#define MAX_NFFT 256
#define N_ALL_FFT 4
int ALL_N_FFT[N_ALL_FFT] = {256, 128, 64, 32};
int N_FFT;
kiss_fft_cpx in_buffer[MAX_NFFT];
kiss_fft_cpx out_buffer[MAX_NFFT];


void setup() {
  //start the serial
  Serial.begin(115200);

  //delay to allow the serial to really get going
  delay(2000);

  //prepare for blinky light
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

}

#define N_LOOP 10
int count = 0;
long start_micros = 0;
long accumulated_micros = 0;
int led_status = LOW;
kiss_fft_cfg my_fft_cfg;
void loop() {

  for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
    N_FFT = ALL_N_FFT[I_N_FFT];

    //Serial << "Allocating memory for N = " << N_FFT << '\n';
    int is_ifft = 0;
    if (my_fft_cfg) KISS_FFT_FREE(my_fft_cfg); //de-allocate the memory prior to making new allocations
    my_fft_cfg = kiss_fft_alloc(N_FFT,is_ifft,NULL, NULL); 
    //Serial << "my_fft_cfg = " << ((int)my_fft_cfg) << '\n';

    accumulated_micros = 0;
    count = 0;

    //Serial << "Beginning N_LOOP = " << N_LOOP << '\n';
    while (count < N_LOOP) {
      led_status = HIGH;
      digitalWrite(ledPin, led_status);
      count++;

      //prepare input data
      for (int i = 0; i < N_FFT; i++) {
        in_buffer[i].r = (kiss_fft_scalar)(i % 8); //real
        in_buffer[i].i = (kiss_fft_scalar)0; //imaginary
      }

      //start timer
      start_micros = micros();

      //do fft
      kiss_fft(my_fft_cfg,in_buffer,out_buffer);

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

