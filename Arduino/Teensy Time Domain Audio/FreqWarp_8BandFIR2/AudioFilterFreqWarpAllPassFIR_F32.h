
/*
   AudioEffectMine

   Created: Chip Audette, December 2016
   Purpose; Here is the skeleton of a audio processing algorithm that will
       (hopefully) make it easier for people to start making their own 
       algorithm.
  
   This processes a single stream fo audio data (ie, it is mono)

   MIT License.  use at your own risk.
*/



#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>
#include <math.h>

#include <Tympan_Library.h> 
#include "utility/BTNRH_rfft.h"

class FreqWarpAllPass 
{
  public:
    FreqWarpAllPass(void) { setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT); }
    FreqWarpAllPass(float _fs_Hz) { setSampleRate_Hz(_fs_Hz);  }
    
    void setSampleRate_Hz(float fs_Hz) {
      //https://www.dsprelated.com/freebooks/sasp/Bilinear_Frequency_Warping_Audio_Spectrum.html
      float fs_kHz = fs_Hz / 1000;
      rho = 1.0674f *sqrtf( 2.0/M_PI * atan(0.06583f * fs_kHz) ) - 0.1916f;
      //b[0] = -rho; b[1] = 1.0f;
      //a[0] = 1.0f;  a[1] = -rho;
    }
    
    float update(const float &in_val) {
      float out_val = ((-rho)*in_val) + prev_in + (rho*prev_out);
      prev_in = in_val; prev_out = out_val; //save states for next time
      return out_val;
    }
    float rho;
    
  protected:
    //float b[2];
    //float a[2];
    float prev_in=0.0f;
    float prev_out=0.0f;


};

#define N_FREQWARP_SAMP (16)
#define N_FREQWARP_ALLPASS (15)  //N-1
#define N_FREQWARP_FIR (9)      //N/2+1
class AudioFilterFreqWarpAllPassFIR_F32 : public AudioStream_F32
{
   public:
    //constructor
    AudioFilterFreqWarpAllPassFIR_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {
      setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
      //setFirCoeff();
      setFirCoeff_viaFFT();
    }
    AudioFilterFreqWarpAllPassFIR_F32(AudioSettings_F32 settings) : AudioStream_F32(1, inputQueueArray_f32) {
      setSampleRate_Hz(settings.sample_rate_Hz);
      //setFirCoeff();
      setFirCoeff_viaFFT();
    }
    void setSampleRate_Hz(float fs_Hz) {
      for (int Ifilt=0; Ifilt < N_FREQWARP_ALLPASS; Ifilt++) freqWarpAllPass[Ifilt].setSampleRate_Hz(fs_Hz);
      //setFirCoeff(); //not needed...not dependent upon sample rate
      setFirCoeff_viaFFT();
    };


    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
      //Serial.println("AudioEffectMine_F32: doing update()");  //for debugging.
      audio_block_f32_t *audio_block;
      audio_block = AudioStream_F32::receiveReadOnly_f32();
      if (!audio_block) return;

      //do your work
      applyMyAlgorithm(audio_block);

      //assume that applyMyAlgorithm transmitted the block so that we just need to release audio_block
      AudioStream_F32::release(audio_block);
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
    // Here is where you can add your algorithm.
    // This function gets called block-wise
    void applyMyAlgorithm(audio_block_f32_t *audio_block) {
      float out_val=0.0f;

      //allocate output block for each output channel
      audio_block_f32_t *out_blocks[N_FREQWARP_FIR];
      for (int I_fir=0; I_fir < N_FREQWARP_FIR; I_fir++) {
        out_blocks[I_fir] = AudioStream_F32::allocate_f32();
        if (out_blocks[I_fir]==NULL) {
          //failed to allocate! release any blocks that have been allocated, then return;
          if (I_fir > 0) { for (int J=I_fir-1; J >= 0; J--) AudioStream_F32::release(out_blocks[J]); }
          if (Serial) {
            Serial.print("AudioFilterFreqWarpAllPassFIR: applyMyAlgorithm: could only allocate ");
            Serial.print(I_fir+1);
            Serial.println(" output blocks.  Returning...");
          }
          return;
        }
        out_blocks[I_fir]->length = audio_block->length;  //set the length of the output block
      }

      //loop over each sample and do something on a point-by-point basis (when it cannot be done as a block)
      for (int Isamp=0; Isamp < (audio_block->length); Isamp++) {
        
        // increment the delay line
        incrementDelayLine(delay_line,audio_block->data[Isamp],N_FREQWARP_SAMP);
        
        //apply each FIR filter (convolution with the delay line)
        for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
          out_val = 0.0f; //initialize
          for (int I=0; I < N_FREQWARP_SAMP; I++) { //loop over each sample in the delay line
            out_val += (delay_line[I]*all_FIR_coeff[I_fir][I]);
          }
          out_blocks[I_fir]->data[Isamp] = out_val;
        }
      }

      //transmit each block
      for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
        AudioStream_F32::transmit(out_blocks[I_fir],I_fir);
        AudioStream_F32::release(out_blocks[I_fir]);
      }
      
    } //end of applyMyAlgorithms

  void setFirCoeff_viaFFT(void) {
      if (Serial) Serial.println("AudioFilterFreqWarpAllPassFIR_F32: setFirCoeff_viaFFT: start.");
    
      const int n_fft = N_FREQWARP_SAMP ;
      float win[n_fft];
      const int N_NYQUIST_REAL = (n_fft/2+1);
      const int N_NYQUIST_COMPLEX = N_NYQUIST_REAL*2;
      float foo_vector[N_NYQUIST_COMPLEX];
      //float foo_b[N_FREQWARP_SAMP];

      //make the window
      calcWindow(n_fft, win);

      //loop over each FIR filter
      for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
        for (int I=0; I<N_NYQUIST_COMPLEX; I++) foo_vector[I]=0.0f;  //clear

        //desired impulse response
        int ind_middle = N_NYQUIST_REAL-1;
        foo_vector[ind_middle-1]=1.0f;  //desired impulse results
        //foo_vector[ind_middle]=0.5;  //desired impulse results

        //convert to frequency domain (result is complelx, but only up to nyquist bin
        BTNRH_FFT::cha_fft_rc(foo_vector, n_fft);

        //keep just the bin(s) that we want
        int targ_bin = I_fir;  //here's the bin that we want to keep
        for (int Ibin=0; Ibin < N_NYQUIST_REAL; Ibin++) {
          if (Ibin != targ_bin) {
            //zero out the non-target bins
            foo_vector[2*Ibin] = 0.0f;   // zero out the real
            foo_vector[2*Ibin+1] = 0.0f; // zero out the complex
          }
        }

        //inverse fft to come back into the time domain (not complex)
        BTNRH_FFT::cha_fft_cr(foo_vector, n_fft); //IFFT back into the time domain

        //apply window and save
        if (Serial) {
          Serial.print("setFirCoeff_viaFFT: FIR ");
          Serial.print(I_fir);
          Serial.print(": ");
        }
        for (int I=0; I < n_fft; I++) {
          //all_FIR_coeff[I_fir][I] = foo_vector[I];
          all_FIR_coeff[I_fir][I] = foo_vector[I]*win[I];
          if (Serial) {
             Serial.print(foo_vector[I]);
             Serial.print(", ");
          }
        }
        if (Serial) Serial.println();
//
//        //do a warped delay line on the coefficients
//        float foo_delay_line[N_FREQWARP_SAMP];
//        for (int I=0; I<N_FREQWARP_SAMP; I++) foo_delay_line[I] = 0.0f;
//        for (int I=0; I<N_FREQWARP_SAMP; I++) {
//          incrementDelayLine(foo_delay_line,all_FIR_coeff[I_fir][I],N_FREQWARP_SAMP);
//        }
//
//        //copy warped signal out as the FIR coefficients
//        for (int I=0; I < N_FREQWARP_SAMP; I++) all_FIR_coeff[I_fir][I] = foo_delay_line[I];
//          
      }
    }
    void printRho(void) {
      if (Serial) {
        Serial.print("AudioFilterFreqWarpAllPassFIR_F32: rho = ");
        Serial.println(freqWarpAllPass[0].rho);
      }
    }
  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    FreqWarpAllPass freqWarpAllPass[N_FREQWARP_ALLPASS];
    float delay_line[N_FREQWARP_SAMP];
    float all_FIR_coeff[N_FREQWARP_FIR][N_FREQWARP_SAMP];

  

    void calcWindow(int n,float *win) {
      //if (Serial) Serial.print("calcWindow: ");
      for (int I = 0; I < n; I++) {
        //hanning
        win[I] = (1.0 - cos(2.0*M_PI*((float(I))/((float)(n-1)))))/2.0f;  //hanning
        win[I] *= 2.0f;  //make up for lost gain in windowing;
        //if (Serial) { Serial.print(win[I]); Serial.print(", "); }
      }
      //if (Serial) Serial.println();
    }

    void incrementDelayLine(float *delay_line_state, const float &new_val, const int N_delay_line) {          
      //increment the delay line
      #define INCREMENT_MODE 2
      #if (INCREMENT_MODE == 1)
        //frequency warped delay line...start and the end and work backwards
        for (int Idelay=N_delay_line-1; Idelay > 0; Idelay--) {
          delay_line_state[Idelay] = freqWarpAllPass[Idelay-1].update(delay_line_state[Idelay-1]); //delay line is one longer than freqWarpAllPass
        }
        delay_line_state[0] = new_val;//put the newest value into the start of the delay line
      #elif (INCREMENT_MODE == 2)
         delay_line_state[0] = new_val;//put the newest value into the start of the delay line
         for (int Idelay=1; Idelay < N_delay_line; Idelay++) {
          delay_line_state[Idelay] = freqWarpAllPass[Idelay-1].update(delay_line_state[Idelay-1]); //delay line is one longer than freqWarpAllPass
        }
      #elif (INCREMENT_MODE == 0)
        //regular (linear) delay line         
        for (int Idelay=N_delay_line-1; Idelay > 0; Idelay--) {
          delay_line_state[Idelay]=delay_line_state[Idelay-1];  //shift by one
        }
        delay_line_state[0] = new_val;//put the newest value into the start of the delay line
      #endif

    }
    
    void setFirCoeff(void) {
      float phase_rad, dphase_rad;
      float foo_b, win[N_FREQWARP_SAMP];

      if (Serial) Serial.print("AudioFilterFreqWarpAllPassFIR: setFirCoeff: computing coefficients...");

      //make window
      calcWindow(N_FREQWARP_SAMP, win);

      //make each FIR filter
      for (int I_fir=0; I_fir < N_FREQWARP_FIR; I_fir++) {
        
        //each FIR should be (in effect) one FFT bin higher in frequency
        //dphase_rad = ((float)I_fir) * (2.0f*M_PI) / ((float)N_FREQWARP_SAMP);
        dphase_rad = (2.0f*M_PI) * ((float)(I_fir+1)) / ((float)N_FREQWARP_SAMP);  //skip the DC term
        
        for (int I = 0; I < N_FREQWARP_SAMP; I++) {
          phase_rad = ((float)I)*dphase_rad;
          foo_b = cosf((float)phase_rad);
          foo_b /= ((float)N_FREQWARP_SAMP);
          
 
          //scale the coefficient and save
          foo_b = foo_b*win[I];
          //if ( (I_fir > 0) && (I_fir < (N_FREQWARP_FIR-1)) ) {
          //  foo_b *= 2.0f;
          //}
          all_FIR_coeff[I_fir][I] = foo_b;

          //if ((I_fir == 0) && (I<2)) {
          //  all_FIR_coeff[I_fir][I] = 0.5f;
          //} else {
          //  all_FIR_coeff[I_fir][I] = 0.0f;
          //}    
        }
      }
    }

};  //end class definition for AudioEffectMine_F32

