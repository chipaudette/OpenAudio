
#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioEffectGain_Float : public AudioStream_Float //AudioStream_Float is in AudioFloatProcessing.h
{
  public:
    AudioEffectGain_Float(void) {};
    void update(audio_float_block_t *block) { //get and put audio data to block
      //for (int i = 0; i < block->length; i++) block->data[i] = gain * (block->data[i]);
      arm_scale_f32(block->data, gain, block->data, block->length); //use ARM DSP for speed!
    }
    void setGain(float g) { gain = g; }
    void setGain_dB(float gain_dB) {
      float gain = pow(10.0, gain_dB / 20.0);
      setGain(gain);
    }
    float getGain(void) { return gain; }
    float getGain_dB(void) { return 20.0*log10(gain); }
  private:
    float gain = 1.0; //default value
};

