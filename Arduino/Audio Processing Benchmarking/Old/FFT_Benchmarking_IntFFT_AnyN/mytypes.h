
typedef int fixed;
#include <math.h>

class Int_FFT_cfg {
  public:
    Int_FFT_cfg() {};
  
    Int_FFT_cfg(int n) {
      N_FFT = n;

      //allocate memory for the table;
      Sinewave = new int[N_FFT];

      //figure out the factor of two for this FFT
      LOG2_N_WAVE = 0;
      while (n > 0) {
        LOG2_N_WAVE++;
        n = n >> 1;  //shift down (ie, divide by two)
      }

      //create sine lookup table
      createSineLookupTable();
    }
  ~Int_FFT_cfg(void) {
    delete [] Sinewave;
  }

  int* Sinewave;
  int N_FFT=0;
  int LOG2_N_WAVE=0;
    
  private:

    void createSineLookupTable(void) {
      const float MAX_VAL = 32768.0;
      const float f_N_FFT = (float)N_FFT;
      for (int i=0; i<N_FFT; i++) {
        Sinewave[i] = (int)(MAX_VAL*sin(2*M_PI*((float)i)/f_N_FFT));
      }
    }
};


