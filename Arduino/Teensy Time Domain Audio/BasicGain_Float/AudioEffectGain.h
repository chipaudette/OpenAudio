
#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioEffectGain_F32 : public AudioStream_F32 //AudioStream_F32 is in AudioFloatProcessing.h
{
  public:
    AudioEffectGain_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    //void update(audio_block_f32_t *block) { //get and put audio data to block
    //  //for (int i = 0; i < block->length; i++) block->data[i] = gain * (block->data[i]);
    //  arm_scale_f32(block->data, gain, block->data, block->length); //use ARM DSP for speed!
    //}
    void update(void) {
      //Serial.println("AudioEffectGain_F32: updating.");
      audio_block_f32_t *block;
      block = AudioStream_F32::receiveWritable_f32();
      if (!block) return;

      //apply the gain
      //for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = gain * (block->data[i]);
      arm_scale_f32(block->data, gain, block->data, block->length); //use ARM DSP for speed!

      //transmit the block and be done
      AudioStream_F32::transmit(block);
      AudioStream_F32::release(block);
    }
    //void update(void) {}; //vestigal from AudioStream portion of AudioStream_F32
    void setGain(float g) { gain = g; }
    void setGain_dB(float gain_dB) {
      float gain = pow(10.0, gain_dB / 20.0);
      setGain(gain);
    }
    float getGain(void) { return gain; }
    float getGain_dB(void) { return 20.0*log10(gain); }
  private:
    audio_block_f32_t *inputQueueArray_f32[1];
    float gain = 1.0; //default value
};

