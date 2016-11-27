#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioConvertInt16ToFloat : public AudioStream_Float //receive Int and transmits Float
{
  public:
    AudioConvertInt16ToFloat(void) : AudioStream_Float(1, inputQueueArray_Float) { };
    void update(void) {
      //get the Int16 block
      audio_block_t *int_block;
      int_block = AudioStream::receiveWritable(); //int16 data block
      if (!int_block) return;

      //allocate a float block
      audio_float_block_t *float_block;
      float_block = AudioStream_Float::allocate_float(); 
      if (float_block == NULL) return;
      
      //convert to float
      convertAudio_Int16toFloat(int_block, float_block, AUDIO_BLOCK_SAMPLES);

      //transmit the audio and return it to the system
      AudioStream_Float::transmit(float_block,0);
      AudioStream_Float::release(float_block);
      AudioStream::release(int_block);
    };
    
  private:
    audio_float_block_t *inputQueueArray_Float[1];
    const float MAX_INT = 32678.0;
    void convertAudio_Int16toFloat(audio_block_t *in, audio_float_block_t *out, int len) {
      //for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i])/MAX_INT;
      for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i]);
      arm_scale_f32(out->data, 1.0/MAX_INT, out->data, out->length); //divide by 32678 to get -1.0 to +1.0
    }
};


class AudioConvertFloatToInt16 : public AudioStream_Float //receive Float and transmits Int
{
  public:
    AudioConvertFloatToInt16(void) : AudioStream_Float(1, inputQueueArray_Float) {};
    void update(void) {
      //get the float block
      audio_float_block_t *float_block;
      float_block = AudioStream_Float::receiveWritable_Float(); //float data block
      if (!float_block) return;

      //allocate a Int16 block
      audio_block_t *int_block;
      int_block = AudioStream::allocate(); 
      if (int_block == NULL) return;
      
      //convert back to int16
      convertAudio_FloatToInt16(float_block, int_block, AUDIO_BLOCK_SAMPLES);

      //return audio to the system
      AudioStream::transmit(int_block);
      AudioStream::release(int_block);
      AudioStream_Float::release(float_block);
    };
    
  private:
    audio_float_block_t *inputQueueArray_Float[1];
    const float MAX_INT = 32678.0;
    void convertAudio_FloatToInt16(audio_float_block_t *in, audio_block_t *out, int len) {
      for (int i = 0; i < len; i++) {
        out->data[i] = (int16_t)(max(min( (in->data[i] * MAX_INT), MAX_INT), -MAX_INT));
      }
    }
};

