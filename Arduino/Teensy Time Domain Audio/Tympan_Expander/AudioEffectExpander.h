/*
 * AudioEffectExampander_F32: Expander
 * 
 * Created: Chip Audette (OpenAudio) Mar 2017
 * Derived From: AudioEffectCompWDRCF32
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _AudioEffectExpander_F32
#define _AudioEffectExpander_F32

#include <Arduino.h>
#include <AudioStream_F32.h>
#include <arm_math.h>
#include <AudioCalcEnvelope_F32.h>
#include "AudioCalcGainWDRC_F32.h"  //has definition of CHA_WDRC
//#include "utility/textAndStringUtils.h"

#define db2(x) (AudioCalcGainWDRC_F32::db2(x))
#define undb2(x) (AudioCalcGainWDRC_F32::undb2(x))

class AudioEffectExpander_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName: Expander
  public:
    AudioEffectExpander_F32(void): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
      setDefaultValues();
    }

    AudioEffectExpander_F32(AudioSettings_F32 settings): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setSampleRate_Hz(settings.sample_rate_Hz);
      setDefaultValues();
    }

    //here is the method called automatically by the audio library
    void update(void) {
      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;
      
      //allocate memory for the output of our algorithm
      audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
      if (!out_block) return;
      out_block->length = in_block->length;
      
      //do the algorithm
      do_expander(in_block->data, out_block->data, in_block->length);
      
      // transmit the block and release memory
      AudioStream_F32::transmit(out_block); // send the FIR output
      AudioStream_F32::release(out_block);
      AudioStream_F32::release(in_block);
    }


     void do_expander(float *x, float *y, int n)    
     //x, input, audio waveform data
     //y, output, audio waveform data after compression
     //n, input, number of samples in this audio block
    {        
        // find smoothed envelope
        audio_block_f32_t *envelope_block = AudioStream_F32::allocate_f32();
        if (!envelope_block) return;
        calcEnvelope.smooth_env(x, envelope_block->data, n);
        envelope_block->length = n;
        
        //calculate gain
        audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
        if (!gain_block) return;
        calcGainFromEnvelope(envelope_block->data, gain_block->data, n);
        gain_block->length = n;

        //apply gain
        arm_mult_f32(x, gain_block->data, y, n);

        // release memory
        AudioStream_F32::release(envelope_block);
        AudioStream_F32::release(gain_block);
    }

    void calcGainFromEnvelope(float *env, float *gain_out, const int n)  {
      //env = input, signal envelope (not the envelope of the power, but the envelope of the signal itslef)
      //gain = output, the gain in natural units (not power, not dB)
      //n = input, number of samples to process in each vector
  
      //prepare intermediate data block
      audio_block_f32_t *env_dBFS_block = AudioStream_F32::allocate_f32();
      if (!env_dBFS_block) return;
      env_dBFS_block->length = n;
  
      //convert to dB
      for (int k=0; k < n; k++) env_dBFS_block->data[k] = db2(env[k]); //maxdb in the private section 
      
      // compute expansion
      float32_t diff_rel_thresh_dB=0.f;
      for (int k=0; k < n; k++) {
        gain_out[k] = linear_gain_dB; //default gain
        diff_rel_thresh_dB = env_dBFS_block->data[k] - thresh_dBFS;
        if (diff_rel_thresh_dB < 0.f) {
          //expand!
          gain_out[k] += (expand_ratio * diff_rel_thresh_dB);  //diff_rel_thresh_dB will be negative
        }

        //convert from dB into linear gain
        gain_out[k] = undb2(gain_out[k]);
      }
      AudioStream_F32::release(env_dBFS_block);
    }


    void setDefaultValues(void) {
      //set default values...
      float att_ms = 5.f, rel_ms = 50.f;
      float er = 3.f, thrsh_dBFS = -60.f;
      float lin_gain_dB = 0.0;
      setParams(att_ms, rel_ms, thrsh_dBFS, er, lin_gain_dB); 
      
    }

    void setGain_dB(float gain_dB) {
      linear_gain_dB = gain_dB;
    }

    //set all of the user parameters for the compressor
    //assumes that the sample rate has already been set!!!
    void setParams(float attack_ms, float release_ms, float thrsh_dBFS, float er, float lin_gain_dB) {
      
      //configure the envelope calculator...assumes that the sample rate has already been set!
      calcEnvelope.setAttackRelease_msec(attack_ms,release_ms);

      //set local private members
      setGain_dB(lin_gain_dB);
      thresh_dBFS = thrsh_dBFS;
      expand_ratio = er;
    }

    void setSampleRate_Hz(const float _fs_Hz) {
      //pass this data on to its components that care
      given_sample_rate_Hz = _fs_Hz;
      calcEnvelope.setSampleRate_Hz(_fs_Hz);
    }

    float getCurrentLevel_dB(void) { return db2(calcEnvelope.getCurrentLevel()); }  //this is 20*log10(abs(signal)) after the envelope smoothing

    AudioCalcEnvelope_F32 calcEnvelope;

    
  private:
    audio_block_f32_t *inputQueueArray[1];
    float given_sample_rate_Hz;
    float linear_gain_dB;
    float thresh_dBFS;
    float expand_ratio;
};


#endif
    

