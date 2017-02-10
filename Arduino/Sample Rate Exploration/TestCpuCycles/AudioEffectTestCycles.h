

#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>

    
class AudioEffectTestCycles : public AudioStream
{
   public:
    //constructor
    AudioEffectTestCycles(void) : AudioStream(1, inputQueueArray) {
      //do any setup activities here
    };

   
     
    //here's the method that is called automatically by the Teensy Audio Library every data block
    void update(void) {
      audio_block_t *audio_block;
      audio_block = receiveWritable();
      if (!audio_block) return;

      //do work here
      for (int i=0; i<AUDIO_BLOCK_SAMPLES;i++) {
        float32_t val = audio_block->data[i];
        val /= 32768.0f;
        for (int j=0; j < (int)(user_parameter); j++) {
          val = expf(val);
        }
        audio_block->data[i] = (int16_t)(val*32768.0f);
      }
      
      ///transmit the block and release memory
      transmit(audio_block);
      release(audio_block);
    }
     

    // Call this method in your main program if you want to set the value of your user parameter. 
    // The user parameter can be used in your algorithm above.  The user_parameter variable was
    // created in the "private:" section of this class, which appears a little later in this file.
    // Feel free to create more user parameters (and to use better names for your variables)
    // for use in this class.
    float32_t setUserParameter(float val) {
      return user_parameter = 100.0*val;
    }
 
  private:
    //state-related variables
    audio_block_t *inputQueueArray[1]; //memory pointer for the input to this module


    //this value can be set from the outside (such as from the potentiometer) to control
    //a parameter within your algorithm
    float32_t user_parameter = 0.0;   

};  //end class definition

