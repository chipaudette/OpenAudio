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

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include <AudioStream_F32.h>

class AudioEffectCompressor_F32 : public AudioStream_F32
{
  public:
    //constructor
    AudioEffectCompressor_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};

    //here's the method that does all the work
    void update(void) {
      //Serial.println("AudioEffectGain_F32: updating.");  //for debugging.
      audio_block_f32_t *block;
      block = AudioStream_F32::receiveWritable_f32();
      if (!block) return;

      //apply the pre-gain
      arm_scale_f32(block->data, pre_gain, block->data, block->length); //use ARM DSP for speed!

      //transmit the block and be done
      AudioStream_F32::transmit(block);
      AudioStream_F32::release(block);
    }

    //methods to set parameters of this module
    void resetGain(void) { gain = 1.0; }
    void setPreGain(float g) { pre_gain = g; }
    void setPreGain_dB(float gain_dB) {
      float pre_gain = pow(10.0, gain_dB / 20.0);
      setGain(pre_gain);
    }
    void setAttack_sec(float a) {
      attack_sec = a;
      updateAttackConstant();
    }
    void setRelease_sec(float r) {
      release_sec = r;
      updateReleaseConstant();
    }
    void setThresh(float t) { thresh = t; }
    void setThresh_dB(float thresh_dB) { setThresh(pow(10.0, gain_dB / 20.0)); }

    //methods to return information about this module
    float getGain(void) { return gain; }
    float getGain_dB(void) { return 20.0*log10(gain); }
    float getPreGain_dB(void) { return 20*.0*log10(pre_gain); }
    float getAttack_sec(void) { return attack_sec; }
    float getRelease_sec(void) { return release_sec; }
    float getThresh_dBFS(void) { return 20.0*log10(thresh_FS); }
    float getCompRatio(void) { return comp_ratio; }
    
  private:
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    float gain = 1.0; //current gain value
    float attack_sec = 0.005; //attack time
    float release_sec = 0.100; //release time
    float attack_const[2], release_const[];  //these will be tied to attack_sec and release_sec
    float thresh_FS = 0.1;  //threshold for compression, relative to digital full scale
    float comp_ratio = 5.0;  //compression ratio
    float pre_gain = 1.0;  //gain to apply before the compression
    static void calcUpdateConstant(tau_sec) { return exp(1.0/(AUDIO_SAMPLE_RATE * tau_sec)); }
    void updateAttackConstant(void) { 
      attack_const[0] = 1.0 - calcUpdateConstant(attack_sec); //to apply to new in-coming data value
      attack_const[1] = calcUpdateConstant(attack_sec);      //to applyl to previous output value
    }
    void updateReleaseConstant(void) {
      release_const[0] = 1.0 - calcUpdateConstant(attack_sec); //to apply to new in-coming data value
      release_const[1] = calcUpdateConstant(attack_sec);      //to applyl to previous output value      
    }
    float updateGain(float *latestAudio    ///I"M HERE!!!
};


