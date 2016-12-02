/*
 * AudioEffectCompressor
 * 
 * Created: Chip Audette, December 2016
 * Purpose; Apply single-band dynamic range compression to the audio stream.
 *          Assumes floating-point data.
 *          
 * This processes a single stream fo audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>

class AudioEffectCompressor_F32 : public AudioStream_F32
{
  public:
    //constructor
    AudioEffectCompressor_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {
      setThresh_dBFS(-30.0);
      //updateAttackConstant();
      //updateReleaseConstant();
      resetStates();
    };

    //here's the method that does all the work
    void update(void) {
      //Serial.println("AudioEffectGain_F32: updating.");  //for debugging.
      audio_block_f32_t *audio_block;
      audio_block = AudioStream_F32::receiveWritable_f32();
      if (!audio_block) return;

      //apply the pre-gain...a negative gain value will disable
      if (pre_gain > 0) arm_scale_f32(audio_block->data, pre_gain, audio_block->data, audio_block->length); //use ARM DSP for speed!

      //compute the desired gain
      audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
      calcGain(audio_block,gain_block); //returns through gain_block

      //apply the gain...store it back into audio_block
      arm_mult_f32 (audio_block->data,gain_block->data,audio_block->data,audio_block->length);

      ///transmit the block and release memory
      AudioStream_F32::transmit(audio_block);
      AudioStream_F32::release(audio_block);
      AudioStream_F32::release(gain_block);
    }

    void calcGain(audio_block_f32_t *wav_block, audio_block_f32_t *gain_block) {
      //prepare constants...move this out of this function later
      //float thresh_pow_FS = pow(10.0,0.1*thresh_dBFS);
      float fs_Hz = wav_block->fs_Hz;
      float attack_const = exp(-1.0/(attack_sec*fs_Hz));
      float release_const = exp(-1.0/(release_sec*fs_Hz));
      float comp_ratio_const = (1.0/comp_ratio-1.0);
      float thresh_pow_FS_wCR = pow(thresh_pow_FS,1.0-1.0/comp_ratio);
      
      //calculate the signal power...ie, square the signal:   wav_pow = wav.^2
      audio_block_f32_t *wav_pow_block = AudioStream_F32::allocate_f32();
      arm_mult_f32 (wav_block->data,wav_block->data,wav_pow_block->data,wav_block->length);

      //loop over each sample
      float gain_pow = 1.0;
      for (int i=0; i < wav_pow_block->length; i++) {
        
        //compute target gain
        if (wav_pow_block->data[i] < thresh_pow_FS) {
          //default gain is 1.0
          gain_pow=1.0;
        } else {
          gain_pow = thresh_pow_FS_wCR / pow(wav_pow_block->data[i],comp_ratio_const);
        }

        //smooth the gain in time
        if (gain_pow < prev_gain_pow) {
          //attack phase
          gain_pow = prev_gain_pow*attack_const + gain_pow*(1.0-attack_const);
        } else {
          //release phase
          gain_pow = prev_gain_pow*release_const + gain_pow*(1.0-release_const);
        }

        //compute gain from gain^2
        arm_sqrt_f32(gain_pow,&(gain_block->data[i]));

        //save value for the next loop
        prev_gain_pow = gain_pow;
      }

      //free up the memory and return
      release(wav_pow_block);
      return;

    }

    //methods to set parameters of this module
    void resetStates(void) { prev_gain_pow = 1.0; }
    void setPreGain(float g) { pre_gain = g; }
    void setPreGain_dB(float gain_dB) { setPreGain(pow(10.0, gain_dB / 20.0)); }
    void setCompressionRatio(float cr) { comp_ratio = max(0.001,cr); } //limit to positive values
    void setAttack_sec(float a) {
      attack_sec = a;
      //updateAttackConstant();
    }
    void setRelease_sec(float r) {
      release_sec = r;
      //updateReleaseConstant();
    }
    void setThreshPow(float t_pow) { thresh_pow_FS = t_pow; }
    void setThresh_dBFS(float thresh_dBFS) { setThreshPow(pow(10.0, thresh_dBFS / 10.0)); }

    //methods to return information about this module
    float getPreGain_dB(void) { return 20*.0*log10(pre_gain); }
    float getAttack_sec(void) { return attack_sec; }
    float getRelease_sec(void) { return release_sec; }
    float getThresh_dBFS(void) { return 10.0*log10(thresh_pow_FS); }
    float getCompressionRatio(void) { return comp_ratio; }
    
  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    float prev_gain_pow = 1.0; //last gain^2 used

    //settings
    float attack_sec = 0.005; //attack time
    float release_sec = 0.100; //release time
    //float attack_const[2], release_const[];  //these will be tied to attack_sec and release_sec
    float thresh_pow_FS = 0.01;  //threshold for compression, relative to digital full scale
    float comp_ratio = 5.0;  //compression ratio
    float pre_gain = 1.0;  //gain to apply before the compression
    //static float calcUpdateConstant(float tau_sec) { 
      //standard definition of exponential time constant, applied to a digital system.
      //stated in hearing aid terms, gets within 2dB of final gain after the given tau
    //  return exp(-1.0/(AUDIO_SAMPLE_RATE * tau_sec)); 
    //} 
    //void updateAttackConstant(void) { 
    //  attack_const[0] = 1.0 - calcUpdateConstant(attack_sec); //to apply to new in-coming data value
    //  attack_const[1] = calcUpdateConstant(attack_sec);      //to applyl to previous output value
    //}
    //void updateReleaseConstant(void) {
    //  release_const[0] = 1.0 - calcUpdateConstant(attack_sec); //to apply to new in-coming data value
    //  release_const[1] = calcUpdateConstant(attack_sec);      //to applyl to previous output value      
    //}
    
};


