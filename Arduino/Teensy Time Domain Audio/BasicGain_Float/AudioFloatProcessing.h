#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioFloatProcessing : public AudioStream
{
  public:
    AudioFloatProcessing(void) : AudioStream(1, inputQueueArray) {
      float_block = &float_block_instance;
    };
    void update(void) {
      audio_block_t *int_block;
      int_block = receiveWritable(); //int16 data block
      if (!int_block) return;

      //convert to float
      convertAudio_Int16toFloat(int_block, float_block, AUDIO_BLOCK_SAMPLES);

      //execute all of the daughter processes...passing the audio block from process to process
      for (int i = 0; i < daughter_process_counter; i++) {
        daughter_processes[i]->update(float_block);
      }

      //convert back to int16
      convertAudio_FloatToInt16(float_block, int_block, AUDIO_BLOCK_SAMPLES);

      //return audio to the system
      transmit(int_block);
      release(int_block);
    };

    #define MAX_NUM_DAUGHTERS 8  //maximum number of processes that can be cascaded
    int addProcessing(AudioStream_Float *process) {
      if (daughter_process_counter == (MAX_NUM_DAUGHTERS-1)) return 0;
      daughter_processes[daughter_process_counter] = process;
      daughter_process_counter++;  //increment for next time
      return 1;
    }

  private:
    audio_block_t *inputQueueArray[1];
    audio_float_block_t float_block_instance;
    audio_float_block_t *float_block;
    AudioStream_Float *daughter_processes[MAX_NUM_DAUGHTERS];
    int daughter_process_counter=0;

    const float MAX_INT = 32678.0;
    void convertAudio_Int16toFloat(audio_block_t *in, audio_float_block_t *out, int len) {
      //for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i])/MAX_INT;
      for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i]);
      arm_scale_f32(out->data, 1.0/MAX_INT, out->data, out->length); //divide by 32678 to get -1.0 to +1.0
    }
    void convertAudio_FloatToInt16(audio_float_block_t *in, audio_block_t *out, int len) {
      for (int i = 0; i < len; i++) {
        out->data[i] = (int16_t)(max(min( (in->data[i] * MAX_INT), MAX_INT), -MAX_INT));
      }
    }
};

