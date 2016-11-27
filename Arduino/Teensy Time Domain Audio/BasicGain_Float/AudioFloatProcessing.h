#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioStream_Float;
class AudioConnection_Float;

//create a new structure to hold audio as floating point values.
//modeled on the existing teensy audio block struct, which uses Int16
//https://github.com/PaulStoffregen/cores/blob/268848cdb0121f26b7ef6b82b4fb54abbe465427/teensy3/AudioStream.h
typedef struct audio_float_block_struct {
  unsigned char ref_count;
  unsigned char memory_pool_index;
  unsigned char reserved1;
  unsigned char reserved2;  
  float data[AUDIO_BLOCK_SAMPLES]; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
  const int length = AUDIO_BLOCK_SAMPLES; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
  const float fs_Hz = AUDIO_SAMPLE_RATE; // AUDIO_SAMPLE_RATE is 44117.64706 from AudioStream.h
} audio_float_block_t;

class AudioConnection_Float
{
  public:
    AudioConnection_Float(AudioStream_Float &source, AudioStream_Float &destination) :
      src(source), dst(destination), src_index(0), dest_index(0),
      next_dest(NULL)
      { connect(); }
    AudioConnection_Float(AudioStream_Float &source, unsigned char sourceOutput,
      AudioStream_Float &destination, unsigned char destinationInput) :
      src(source), dst(destination),
      src_index(sourceOutput), dest_index(destinationInput),
      next_dest(NULL)
      { connect(); }
    friend class AudioStream_Float;
  protected:
    void connect(void);
    AudioStream_Float &src;
    AudioStream_Float &dst;
    unsigned char src_index;
    unsigned char dest_index;
    AudioConnection_Float *next_dest;
};

#define AudioMemory_Float(num) ({ \
  static audio_float_block_t data[num]; \
  AudioStream_Float::initialize_memory(data, num); \
})

class AudioStream_Float {
  public:
    AudioStream_Float(unsigned char ninput, audio_float_block_t **iqueue) :
        num_inputs(ninput), inputQueue(iqueue) {
      active = false;
      destination_list = NULL;
    };
    virtual void update(audio_float_block_t *) = 0; 
  protected:
    bool active;
    unsigned char num_inputs;
    void transmit(audio_float_block_t *block, unsigned char index = 0);
    friend class AudioConnection_Float;
  private:
    AudioConnection_Float *destination_list;
    audio_float_block_t **inputQueue;
};



void AudioStream_Float::transmit(audio_float_block_t *block, unsigned char index)
{
  for (AudioConnection_Float *c = destination_list; c != NULL; c = c->next_dest) {
    if (c->src_index == index) {
      if (c->dst.inputQueue[c->dest_index] == NULL) {
        c->dst.inputQueue[c->dest_index] = block;
        block->ref_count++;
      }
    }
  }      
}

void AudioConnection_Float::connect(void) {
  AudioConnection_Float *p;
  if (dest_index > dst.num_inputs) return;
  __disable_irq();
  p = src.destination_list;
  if (p == NULL) {
    src.destination_list = this;
  } else {
    while (p->next_dest) p = p->next_dest;
    p->next_dest = this;
  }
  src.active = true;
  dst.active = true;
  __enable_irq();
}


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

