
// FFT routines
//http://www.physionet.org/physiotools/wfdb/psd/fft.c


#include "types.h"
//include <stdio.h>
#include <math.h>
//include <stdlib.h>
//include <iostream.h>
//using namespace std;


//Add streaming-like output http://playground.arduino.cc/Main/StreamingOutput
template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}

const int ledPin = 13;  //Teensy 3.0+ is on pin 13


#define  MAX_LEN 2048   /* maximum points in FFT */
#define PI  M_PI  /* pi to machine precision, defined in math.h */
#define TWOPI (2.0*PI)


#define N_ALL_FFT 7
int ALL_N_FFT[N_ALL_FFT] = {32, 64, 128, 256, 512, 1024, 2048};
int N_FFT;

//double wsum, rsum,freq, fstep, norm, rmean;
full_res_t wsum, rsum,freq, fstep, norm, rmean;


int N_FFT_2 = MAX_LEN / 2;
int m, n;
int cflag = 1;  //print in complex FFT (rectangular form)
int decimation = 1; //decimation factor for the spectrum
int iflag = 0;  //calculate inverse FFT?
int fflag = 0; //print frequencies?
int len = MAX_LEN;  //perform up to n-point transforms
int nflag = MAX_LEN;        // process in overlapping n-point chunks, output avg
int Nflag = MAX_LEN; // rocess in overlapping n-point chunks, output raw
int nchunks;
int pflag = 0; //print in polar form
int Pflag = 0; //print power spectrum (squared magnitude)
int smooth = 1; //smoothing parameter for the spectrum
int wflag = 0; //apply windowing?
int zflag = 0; //zero mean (1) or detrend (2) or nothing (0)
float c[MAX_LEN];



void fft(float c[])    /* calculate forward FFT */
{
  //Serial.println("fft(): starting");
  //for (m = len; m >= n; m >>= 1)
  //  ;
  //m <<= 1;    /* m is now the smallest power of 2 >= n; this is the
  //       length of the input series (including padding) */

  //assume given N_FFT is a power of two
  //m = N_FFT

  //if (wflag)      /* apply the chosen windowing function */
  // for (i = 0; i < m; i++)
  //    c[i] *= (*window)(i, m);
  //else
  //wsum = N_FFT;
  //norm = sqrt(2.0 / (wsum * n));

  //  if (fflag) fstep = freq / (2.*N_FFT); /* note that fstep is actually half of
  //               the frequency interval;  it is
  //               multiplied by the doubled index i
  //               to obtain the center frequency for
  //               bin (i/2) */

  //Serial << "fft: m = " << m << '\n';
  //realft(c - 1, m / 2, 1); /* perform the FFT;  see Numerical Recipes */
  realft(c - 1, N_FFT_2, 1); /* perform the FFT;  see Numerical Recipes */
}


void ifft(float c[])    /* calculate and print inverse FFT */
{
  int i;

  /* repack IFFT input array */
  n -= 2;
  c[1] = c[n];     //indexes from one
  if (iflag < 0) {    /* convert polar form input to rectangular */
    for (i = 2; i < n; i += 2) {
      float im;

      im = c[i] * sin(c[i + 1]);
      c[i] *= cos(c[i + 1]);
      c[i + 1] = im;
    }
  }
  //realft(c - 1, n / 2, -1);
  realft(c - 1, N_FFT_2, -1);
}

//nn is N_FFT / 2 ?
void four1(float data[], int nn, int isign)
{
  int n, mmax, m, j, istep, i;
  //double wtemp, wr, wpr, wpi, wi, theta;
  full_res_t wtemp, wr, wpr, wpi, wi, theta;
  float tempr, tempi;

  n = nn << 1;
  j = 1;
  for (i = 1; i < n; i += 2) {
    if (j > i) {
      tempr = data[j];     data[j] = data[i];     data[i] = tempr;
      tempr = data[j + 1]; data[j + 1] = data[i + 1]; data[i + 1] = tempr;
    }
    m = n >> 1;
    while (m >= 2 && j > m) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }
  mmax = 2;
  while (n > mmax) {
    istep = 2 * mmax;
    theta = TWOPI / (isign * mmax);
    wtemp = sin(0.5 * theta);
    wpr = -2.0 * wtemp * wtemp;
    wpi = sin(theta);
    wr = 1.0;
    wi = 0.0;
    for (m = 1; m < mmax; m += 2) {
      for (i = m; i <= n; i += istep) {
        j = i + mmax;
        tempr = wr * data[j]   - wi * data[j + 1];
        tempi = wr * data[j + 1] + wi * data[j];
        data[j]   = data[i]   - tempr;
        data[j + 1] = data[i + 1] - tempi;
        data[i] += tempr;
        data[i + 1] += tempi;
      }
      wr = (wtemp = wr) * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
    }
    mmax = istep;
  }
}


//float data[];
//int n,isign;
void realft(float data[], int n, int isign)
{
  int i, i1, i2, i3, i4, n2p3;
  float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
  //double wr, wi, wpr, wpi, wtemp, theta;
  full_res_t wr, wi, wpr, wpi, wtemp, theta;
  //void four1(data,n,isign);

  theta = PI / (full_res_t) n;
  if (isign == 1) {
    c2 = -0.5;
    four1(data, n, 1);
  }
  else {
    c2 = 0.5;
    theta = -theta;
  }
  wtemp = sin(0.5 * theta);
  wpr = -2.0 * wtemp * wtemp;
  wpi = sin(theta);
  wr = 1.0 + wpr;
  wi = wpi;
  n2p3 = 2 * n + 3;
  for (i = 2; i <= n / 2; i++) {
    i4 = 1 + (i3 = n2p3 - (i2 = 1 + ( i1 = i + i - 1)));
    h1r =  c1 * (data[i1] + data[i3]);
    h1i =  c1 * (data[i2] - data[i4]);
    h2r = -c2 * (data[i2] + data[i4]);
    h2i =  c2 * (data[i1] - data[i3]);
    data[i1] =  h1r + wr * h2r - wi * h2i;
    data[i2] =  h1i + wr * h2i + wi * h2r;
    data[i3] =  h1r - wr * h2r + wi * h2i;
    data[i4] = -h1i + wr * h2i + wi * h2r;
    wr = (wtemp = wr) * wpr - wi * wpi + wr;
    wi = wi * wpr + wtemp * wpi + wi;
  }
  if (isign == 1) {
    data[1] = (h1r = data[1]) + data[2];
    data[2] = h1r - data[2];
  } else {
    data[1] = c1 * ((h1r = data[1]) + data[2]);
    data[2] = c1 * (h1r - data[2]);
    four1(data, n, -1);
  }
}


void setup() {
  //start the serial
  Serial.begin(115200);

  //delay to allow the serial to really get going
  delay(2000);

  //prepare for blinky light
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

}

#define N_LOOP 100
int count = 0;
long start_micros = 0;
long accumulated_micros = 0;
int led_status = LOW;
void loop() {

  for (int I_N_FFT = 0; I_N_FFT < N_ALL_FFT; I_N_FFT++) {
    N_FFT = ALL_N_FFT[I_N_FFT];

    //Serial << "Beginning N_FFT = " << N_FFT << '\n';

    accumulated_micros = 0;
    count = 0;
    m = N_FFT;
    n = N_FFT;
    N_FFT_2 = N_FFT / 2;
    //wsum = (float)N_FFT;  //sum of windowing function...or N_FFT if no windowing
    //norm = sqrt(2.0 / (wsum * (float)n)); //scale factor for reporting magnitudes
    len = N_FFT;  //perform up to n-point transforms
    nflag = N_FFT;        // process in overlapping n-point chunks, output avg
    Nflag = N_FFT; // rocess in overlapping n-point chunks, output raw

    //Serial << "Beginning N_LOOP = " << N_LOOP << '\n';
    while (count < N_LOOP) {
      led_status = HIGH;
      digitalWrite(ledPin, led_status);
      count++;

      //prepare input data
      for (int i = 0; i < N_FFT; i++) {
        c[i] = float(i % 8);
      }

      //start timer
      start_micros = micros();

      //do fft
      fft(c);

      //finish timer
      accumulated_micros += (micros() - start_micros);
      //Serial << "FFT " << count << " complete.\n";
    }

    if (accumulated_micros > 0) {
      //print the timing results
      Serial.print("N_FFT = ");
      Serial.print(N_FFT);
      Serial.print(", micros per FFT = ");
      Serial.println( ((float)accumulated_micros) / ((float)N_LOOP));
    }

  }

}

