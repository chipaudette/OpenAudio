
#ifndef _AudioEffecFreqDomain_F32_h
#define _AudioEffectFreqDomain_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "OverlappedFFT_F32.h"
//#include "utility/dspinst.h"  //copied from analyze_fft256.cpp.  Do we need this?

class AudioEffectFreqDomain_F32 : public AudioStream_F32
{
  public:
    //constructors
    AudioEffectFreqDomain_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectFreqDomain_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { 
    }
    AudioEffectFreqDomain_F32(const AudioSettings_F32 &settings, const int _N_FFT) : AudioStream_F32(1, inputQueueArray_f32) { 
      setup(settings,_N_FFT);
    }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectFreqDomain_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
    }
       
    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      //setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
      int N_FFT = myFFT.setup(settings,_N_FFT); //hopefully, we got the same N_FFT that we asked for
      if (N_FFT < 1) return N_FFT;
      N_FFT = myIFFT.setup(settings,_N_FFT);  //hopefully, we got the same N_FFT that we asked for
      if (N_FFT < 1) return N_FFT;

      //decide windowing
      (myFFT.getFFTObject()).useHanningWindow(); //applied prior to FFT
      if (myIFFT.getNBuffBlocks() > 3) (myIFFT.getFFTObject()).useHanningWindow(); //window again after FFT

      //print info about setup
      Serial.println("AudioEffectfreqDomain: FFT parameters...");
      Serial.print("    : N_FFT = "); Serial.println(N_FFT);
      Serial.print("    : audio_block_samples = "); Serial.println(settings.audio_block_samples);
      Serial.print("    : N_BUFF_BLOCKS = "); Serial.println(myFFT.getNBuffBlocks());

      //allocate memory to hold frequency domain data
      complex_2N_buffer = new float32_t[2*N_FFT];

      //we're done.  return!
      enabled=1;
      return N_FFT;
    }
    
    virtual void update(void);

  private:
    int enabled=0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    OverlappedFFT_F32 myFFT;
    OverlappedIFFT_F32 myIFFT;
    
};


void AudioEffectFreqDomain_F32::update(void)
{
  //get a pointer to the latest data
  audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_block) return;

  //simply return the audio if this class hasn't been enabled
  if (!enabled) { AudioStream_F32::transmit(in_block); AudioStream_F32::release(in_block); return; }

  //convert to frequency domain
  myFFT.execute(in_block, complex_2N_buffer);
  AudioStream_F32::release(in_block);  //We just passed ownership to myFFT, so release it here.
  
  // ////////////// Do your processing here!!!

  //nothing to do

  // ///////////// End do your processing here

  //call the IFFT
  audio_block_f32_t *out_block = myIFFT.execute(complex_2N_buffer); //out_block is pre-allocated in here.
 

  //send the returned audio block.  Don't issue the release command here because myIFFT will re-use it
  AudioStream_F32::transmit(out_block); //don't release this buffer because myIFFT re-uses it within its own code
  return;
};
#endif
