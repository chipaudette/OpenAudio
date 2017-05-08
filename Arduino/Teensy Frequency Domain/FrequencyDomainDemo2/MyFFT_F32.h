

#ifndef _MyFFT_h
#define _MyFFT_h

//include <math.h>
#include <arm_math.h>

class MyFFT_F32
{
  public:
    MyFFT_F32(void) {};
    MyFFT_F32(const int _N_FFT) {
      setup(_N_FFT);
    }
    MyFFT_F32(const int _N_FFT, const int _is_IFFT) {
      setup(_N_FFT, _is_IFFT);
    }
    ~MyFFT_F32(void) { delete window; };  //destructor

    int setup(const int _N_FFT) {
      int _is_IFFT = 0;
      return setup(_N_FFT,_is_IFFT);
    }
    int setup(const int _N_FFT, const int _is_IFFT) {
      if (!is_valid_N_FFT(_N_FFT)) {
        Serial.println(F("MyFFT_F32: *** ERROR ***"));
        Serial.print(F("    : Cannot use N_FFT = ")); Serial.println(N_FFT);
        Serial.print(F("    : Must be power of 2 between 16 and 2048"));
        return -1;
      }
      N_FFT = _N_FFT;
      is_IFFT = _is_IFFT;

      if ((N_FFT == 16) || (N_FFT == 64) || (N_FFT == 256) || (N_FFT == 1024)) {
        arm_cfft_radix4_init_f32(&fft_inst_r4, N_FFT, is_IFFT, 1); //FFT
        is_rad4 = 1;
      } else {
        arm_cfft_radix2_init_f32(&fft_inst_r2, N_FFT, is_IFFT, 1); //FFT
      }

      //allocate window
      window = new float[N_FFT];
      if (is_IFFT) {
        useRectangularWindow(); //default to no windowing for IFFT
      } else {
        useHanningWindow(); //default to windowing for FFT
      }
      return N_FFT;
    }
    static int is_valid_N_FFT(const int N) {
       if ((N == 16) || (N == 32) || (N == 64) || (N == 128) || 
        (N == 256) || (N == 512) || (N==1024) || (N==2048)) {
          return 1;
        } else {
          return 0;
        }
    }

    void useRectangularWindow(void) {
      flag__useWindow = 0;
      for (int i=0; i < N_FFT; i++) window[i] = 1.0; //cast it to the right type
    }
    void useHanningWindow(void) {
      flag__useWindow = 1;
      if (window != NULL) {
        for (int i=0; i < N_FFT; i++) window[i] = (-0.5f*cosf(2.0f*M_PI*(float)i/(float)N_FFT) + 0.5f); //cast it to the right type
      }
    }
    
    void applyWindowToRealPartOfComplexVector(float32_t *complex_2N_buffer) {
      for (int i=0; i < N_FFT; i++) {
        complex_2N_buffer[2*i] = complex_2N_buffer[2*i]*window[i];
      }
    }
    void applyWindowToRealVector(float32_t *real_N_buffer) {
      for (int i=0; i < N_FFT; i++) {
        real_N_buffer[i] = real_N_buffer[2*i]*window[i];
      }
    }

    virtual void execute(float32_t *complex_2N_buffer) { //interleaved [real,imaginary], total length is 2*N_FFT
      if (N_FFT == 0) return;

      //apply window before FFT (if it is an FFT and not IFFT)
      if ((~is_IFFT) && (flag__useWindow)) applyWindowToRealPartOfComplexVector(complex_2N_buffer);

      //do the FFT
      if (is_rad4) {
        arm_cfft_radix4_f32(&fft_inst_r4, complex_2N_buffer);
      } else {
        arm_cfft_radix2_f32(&fft_inst_r2, complex_2N_buffer);
      }

      //apply window after FFT (if it is an IFFT and not FFT)
      if ((is_IFFT) && (flag__useWindow)) applyWindowToRealPartOfComplexVector(complex_2N_buffer);
    }
    virtual int getNFFT(void) { return N_FFT; };

  private:
    int N_FFT=0;
    int is_IFFT=0;
    int is_rad4=0;
    float *window;
    int flag__useWindow;
    arm_cfft_radix4_instance_f32 fft_inst_r4;
    arm_cfft_radix2_instance_f32 fft_inst_r2;
     
};

class MyIFFT_F32 : public MyFFT_F32  // is basically the same as myFFT, so let's inherent myFFT
{
  public:
    MyIFFT_F32(void) : MyFFT_F32() {};
    MyIFFT_F32(const int _N_FFT): MyFFT_F32() {
      //constructor
      MyIFFT_F32::setup(_N_FFT); //call myFFT's setup routine
    }
    int setup(const int _N_FFT) {
      const int _is_IFFT = 1;
      return MyFFT_F32::setup(_N_FFT, _is_IFFT); //call myFFT's setup routine      
    }
    //all other functions are in myFFT
};

#endif
