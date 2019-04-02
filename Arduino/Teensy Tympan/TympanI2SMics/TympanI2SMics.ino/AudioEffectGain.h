/*
 * AudioEffectGain
 * 
 * Created: Chip Audette, November 2016
 * Purpose; Apply digital gain to the audio data.
 *          
 * This processes a single stream fo audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectGain_h
#define _AudioEffectGain_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include <AudioStream.h>

class AudioEffectGain : public AudioStream
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node  
  public:
    //constructor
    AudioEffectGain(void) : AudioStream(1, inputQueueArray) {};
	  //AudioEffectGain(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {};

    //here's the method that does all the work
    void update(void) {
		//Serial.println("AudioEffectGain_F32: updating.");  //for debugging.
		audio_block_t *block;
		block = AudioStream::receiveWritable();
		if (!block) return;

		//apply the gain
		//for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = gain * (block->data[i]); //non DSP way to do it
		//arm_scale_f32(block->data, gain, block->data, block->length); //use ARM DSP for speed!
    float32_t val=0.0;
    const float32_t MAX_INT = (32678.0-1.0);
    for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
      val = ((float32_t)block->data[i])/MAX_INT;
      val *= gain;
      block->data[i] = (int16_t)(max(min( (val * MAX_INT), MAX_INT), -MAX_INT));
    }

		//transmit the block and be done
		AudioStream::transmit(block);
		AudioStream::release(block);
    }

    //methods to set parameters of this module
    float setGain(float g) { return gain = g;}
    float setGain_dB(float gain_dB) {
      float gain = pow(10.0, gain_dB / 20.0);
      setGain(gain);
	  return getGain_dB();
    }

	//increment the linear gain
    float incrementGain_dB(float increment_dB) {
      return setGain_dB(getGain_dB() + increment_dB);
    }    
	
  //void setSampleRate_Hz(const float _fs_Hz) {};  //unused.  included for interface compatability with fancier gain algorithms
	float getCurrentLevel_dB(void) { return 0.0; };  //meaningless.  included for interface compatibility with fancier gain algorithms
	
    //methods to return information about this module
    float getGain(void) { return gain; }
    float getGain_dB(void) { return 20.0*log10(gain); }
    
  private:
    audio_block_t *inputQueueArray[1]; //memory pointer for the input to this module
    float gain = 1.0; //default value
};

#endif
