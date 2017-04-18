
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

class FreqWarpAllPass 
{
  public:
    FreqWarpAllPass(void) { setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT); }
    FreqWarpAllPass(float _fs_Hz) { setSampleRate_Hz(_fs_Hz);  }
    
    void setSampleRate_Hz(float fs_Hz) {
      //https://www.dsprelated.com/freebooks/sasp/Bilinear_Frequency_Warping_Audio_Spectrum.html
      float fs_kHz = fs_Hz / 1000;
      float rho = 1.0674f *sqrtf( 2.0/M_PI * atan(0.06583f * fs_kHz) ) - 0.1916f;
      b[0] = -rho; b[1] = 1.0f;
      a[0] = 1.0f;  a[1] = -rho;
    }
    
    float update(const float &in_val) {
      float out_val = b[0]*in_val + b[1]*prev_in - a[1]*prev_out;
      prev_in = in_val; prev_out = out_val; //save states for next time
      return out_val;
    }
    
  protected:
    float b[2];
    float a[2];
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
      setFirCoeff();
    }
    AudioFilterFreqWarpAllPassFIR_F32(AudioSettings_F32 settings) : AudioStream_F32(1, inputQueueArray_f32) {
      setSampleRate_Hz(settings.sample_rate_Hz);
      setFirCoeff();
    }
    void setSampleRate_Hz(float fs_Hz) {
      for (int Ifilt=0; Ifilt < N_FREQWARP_ALLPASS; Ifilt++) freqWarpAllPass[Ifilt].setSampleRate_Hz(fs_Hz);
      setFirCoeff(); //not needed...not dependent upon sample rate
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
          return;
        }
        out_blocks[I_fir]->length = audio_block->length;  //set the length of the output block
      }

      //loop over each sample and do something on a point-by-point basis (when it cannot be done as a block)
      for (int Isamp=0; Isamp < (audio_block->length); Isamp++) {
        
        //increment the delay line
        //for (int Idelay=N_FREQWARP_ALLPASS-1; Idelay >= 0; Idelay--) {
        //  delay_line[Idelay+1] = freqWarpAllPass[Idelay].update(delay_line[Idelay]); //delay line is one longer than freqWarpAllPass
        //}
        for (int Idelay=N_FREQWARP_SAMP-1; Idelay > 0; Idelay--) {
          delay_line[Idelay]=delay_line[Idelay-1];
        }
        
        //put the newest value into the start of the delay line
        delay_line[0] = audio_block->data[Isamp];
        
        //loop over each FIR filter
        for (int I_fir = 0; I_fir < N_FREQWARP_FIR; I_fir++) {
          out_blocks[I_fir]->data[Isamp] = 0.0f;
          out_val = 0.0f;
          for (int I=0; I < N_FREQWARP_SAMP; I++) {
            out_val = out_val + (delay_line[I]*all_FIR_coeff[I_fir][I]);
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
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    FreqWarpAllPass freqWarpAllPass[N_FREQWARP_ALLPASS];
    float delay_line[N_FREQWARP_SAMP];
    float all_FIR_coeff[N_FREQWARP_FIR][N_FREQWARP_SAMP];
    
    void setFirCoeff(void) {
      float phase_rad, dphase_rad;
      float foo_b, win;
      for (int I_fir=0; I_fir < N_FREQWARP_FIR; I_fir++) {
        dphase_rad = ((float)I_fir) * (2.0f*M_PI) / ((float)N_FREQWARP_SAMP);
        
        for (int I = 0; I < N_FREQWARP_SAMP; I++) {
          phase_rad = ((float)I)*dphase_rad;
          foo_b = cos(phase_rad);
          
          // scale
          win = 1.0;
          win = 1.f/sqrtf(N_FREQWARP_SAMP);
          if ((I_fir ==0) || (I_fir == (N_FREQWARP_FIR-1))) {
            win = win * 0.5f;
          }
          //win /= (float)N_FREQWARP_SAMP; //make gain of 1.0???
          //win *= (float)(2*I_fir);  //make higher frequencies louder
          
          //hanning windowing
          win *= (1.0 - cos(2.0*M_PI*((float(I))/((float)N_FREQWARP_SAMP-1))))/2.0;  //hanning

          //scale the coefficient and save
          foo_b = foo_b*win;
          all_FIR_coeff[I_fir][I] = foo_b;
        }
      }
    }

};  //end class definition for AudioEffectMine_F32

