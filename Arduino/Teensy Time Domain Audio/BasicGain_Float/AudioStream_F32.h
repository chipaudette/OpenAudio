#include <arm_math.h> //ARM DSP extensions.  for speed!

class AudioStream_F32;
class AudioConnection_F32;

//create a new structure to hold audio as floating point values.
//modeled on the existing teensy audio block struct, which uses Int16
//https://github.com/PaulStoffregen/cores/blob/268848cdb0121f26b7ef6b82b4fb54abbe465427/teensy3/AudioStream.h
typedef struct audio_block_f32_struct {
  unsigned char ref_count;
  unsigned char memory_pool_index;
  unsigned char reserved1;
  unsigned char reserved2;  
  float data[AUDIO_BLOCK_SAMPLES]; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
  const int length = AUDIO_BLOCK_SAMPLES; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
  const float fs_Hz = AUDIO_SAMPLE_RATE; // AUDIO_SAMPLE_RATE is 44117.64706 from AudioStream.h
} audio_block_f32_t;

class AudioConnection_F32
{
  public:
    AudioConnection_F32(AudioStream_F32 &source, AudioStream_F32 &destination) :
      src(source), dst(destination), src_index(0), dest_index(0),
      next_dest(NULL)
      { connect(); }
    AudioConnection_F32(AudioStream_F32 &source, unsigned char sourceOutput,
      AudioStream_F32 &destination, unsigned char destinationInput) :
      src(source), dst(destination),
      src_index(sourceOutput), dest_index(destinationInput),
      next_dest(NULL)
      { connect(); }
    friend class AudioStream_F32;
  protected:
    void connect(void);
    AudioStream_F32 &src;
    AudioStream_F32 &dst;
    unsigned char src_index;
    unsigned char dest_index;
    AudioConnection_F32 *next_dest;
};

#define AudioMemory_F32(num) ({ \
  static audio_block_f32_t data[num]; \
  AudioStream_F32::initialize_f32_memory(data, num); \
})

class AudioStream_F32 : public AudioStream {
  public:
    AudioStream_F32(unsigned char n_input_f32, audio_block_f32_t **iqueue) : AudioStream(1, inputQueueArray_i16), 
        num_inputs_f32(n_input_f32), inputQueue_f32(iqueue) {
      //active_f32 = false;
      destination_list_f32 = NULL;
      for (int i=0; i < n_input_f32; i++) {
        inputQueue_f32[i] = NULL;
      }
    };
    static void initialize_f32_memory(audio_block_f32_t *data, unsigned int num);
    //virtual void update(audio_block_f32_t *) = 0; 
    static uint8_t f32_memory_used;
    static uint8_t f32_memory_used_max;
    
  protected:
    //bool active_f32;
    unsigned char num_inputs_f32;
    static audio_block_f32_t * allocate_f32(void);
    static void release(audio_block_f32_t * block);
    void transmit(audio_block_f32_t *block, unsigned char index = 0);
    audio_block_f32_t * receiveReadOnly_f32(unsigned int index = 0);
    audio_block_f32_t * receiveWritable_f32(unsigned int index = 0);  
    friend class AudioConnection_F32;
  private:
    AudioConnection_F32 *destination_list_f32;
    audio_block_f32_t **inputQueue_f32;
    virtual void update(void) = 0;
    audio_block_t *inputQueueArray_i16[1];  //two for stereo
    static audio_block_f32_t *f32_memory_pool;
    static uint32_t f32_memory_pool_available_mask[6];
};


audio_block_f32_t * AudioStream_F32::f32_memory_pool;
uint32_t AudioStream_F32::f32_memory_pool_available_mask[6];

uint8_t AudioStream_F32::f32_memory_used = 0;
uint8_t AudioStream_F32::f32_memory_used_max = 0;

// Set up the pool of audio data blocks
// placing them all onto the free list
void AudioStream_F32::initialize_f32_memory(audio_block_f32_t *data, unsigned int num)
{
  unsigned int i;

  //Serial.println("AudioStream_F32 initialize_memory");
  //delay(10);
  if (num > 192) num = 192;
  __disable_irq();
  f32_memory_pool = data;
  for (i=0; i < 6; i++) {
    f32_memory_pool_available_mask[i] = 0;
  }
  for (i=0; i < num; i++) {
    f32_memory_pool_available_mask[i >> 5] |= (1 << (i & 0x1F));
  }
  for (i=0; i < num; i++) {
    data[i].memory_pool_index = i;
  }
  __enable_irq();

} // end initialize_memory

// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_block_f32_t * AudioStream_F32::allocate_f32(void)
{
  uint32_t n, index, avail;
  uint32_t *p;
  audio_block_f32_t *block;
  uint8_t used;

  p = f32_memory_pool_available_mask;
  __disable_irq();
  do {
    avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    __enable_irq();
    //Serial.println("alloc_f32:null");
    return NULL;
  } while (0);
  n = __builtin_clz(avail);
  *p = avail & ~(0x80000000 >> n);
  used = f32_memory_used + 1;
  f32_memory_used = used;
  __enable_irq();
  index = p - f32_memory_pool_available_mask;
  block = f32_memory_pool + ((index << 5) + (31 - n));
  block->ref_count = 1;
  if (used > f32_memory_used_max) f32_memory_used_max = used;
  //Serial.print("alloc_f32:");
  //Serial.println((uint32_t)block, HEX);
  return block;
}


// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream_F32::release(audio_block_f32_t *block)
{
  uint32_t mask = (0x80000000 >> (31 - (block->memory_pool_index & 0x1F)));
  uint32_t index = block->memory_pool_index >> 5;

  __disable_irq();
  if (block->ref_count > 1) {
    block->ref_count--;
  } else {
    //Serial.print("release_f32:");
    //Serial.println((uint32_t)block, HEX);
    f32_memory_pool_available_mask[index] |= mask;
    f32_memory_used--;
  }
  __enable_irq();
}

// Transmit an audio data block
// to all streams that connect to an output.  The block
// becomes owned by all the recepients, but also is still
// owned by this object.  Normally, a block must be released
// by the caller after it's transmitted.  This allows the
// caller to transmit to same block to more than 1 output,
// and then release it once after all transmit calls.
void AudioStream_F32::transmit(audio_block_f32_t *block, unsigned char index)
{
  for (AudioConnection_F32 *c = destination_list_f32; c != NULL; c = c->next_dest) {
    if (c->src_index == index) {
      if (c->dst.inputQueue_f32[c->dest_index] == NULL) {
        c->dst.inputQueue_f32[c->dest_index] = block;
        block->ref_count++;
      }
    }
  }      
}

// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_block_f32_t * AudioStream_F32::receiveReadOnly_f32(unsigned int index)
{
  audio_block_f32_t *in;

  if (index >= num_inputs_f32) return NULL;
  in = inputQueue_f32[index];
  inputQueue_f32[index] = NULL;
  return in;
}


// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_block_f32_t * AudioStream_F32::receiveWritable_f32(unsigned int index)
{
  audio_block_f32_t *in, *p;

  if (index >= num_inputs_f32) return NULL;
  in = inputQueue_f32[index];
  inputQueue_f32[index] = NULL;
  if (in && in->ref_count > 1) {
    p = allocate_f32();
    if (p) memcpy(p->data, in->data, sizeof(p->data));
    in->ref_count--;
    in = p;
  }
  return in;
}

void AudioConnection_F32::connect(void) {
  AudioConnection_F32 *p;
  
  if (dest_index > dst.num_inputs_f32) return;
  __disable_irq();
  p = src.destination_list_f32;
  if (p == NULL) {
    src.destination_list_f32 = this;
  } else {
    while (p->next_dest) p = p->next_dest;
    p->next_dest = this;
  }
  src.active = true;
  dst.active = true;
  __enable_irq();
}


class AudioConvert_I16toF32 : public AudioStream_F32 //receive Int and transmits Float
{
  public:
    AudioConvert_I16toF32(void) : AudioStream_F32(1, inputQueueArray_f32) { };
    void update(void) {
      //get the Int16 block
      audio_block_t *int_block;
      int_block = AudioStream::receiveReadOnly(); //int16 data block
      if (!int_block) return;

      //allocate a float block
      audio_block_f32_t *float_block;
      float_block = AudioStream_F32::allocate_f32(); 
      if (float_block == NULL) return;
      
      //convert to float
      convertAudio_I16toF32(int_block, float_block, AUDIO_BLOCK_SAMPLES);

      //transmit the audio and return it to the system
      AudioStream_F32::transmit(float_block,0);
      AudioStream_F32::release(float_block);
      AudioStream::release(int_block);
    };
    
  private:
    audio_block_f32_t *inputQueueArray_f32[1]; //2 for stereo

    static void convertAudio_I16toF32(audio_block_t *in, audio_block_f32_t *out, int len) {
      const float MAX_INT = 32678.0;
      //for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i])/MAX_INT;
      for (int i = 0; i < len; i++) out->data[i] = (float)(in->data[i]);
      arm_scale_f32(out->data, 1.0/MAX_INT, out->data, out->length); //divide by 32678 to get -1.0 to +1.0
    }
};


class AudioConvert_F32toI16 : public AudioStream_F32 //receive Float and transmits Int
{
  public:
    AudioConvert_F32toI16(void) : AudioStream_F32(1, inputQueueArray_Float) {};
    void update(void) {
      //get the float block
      audio_block_f32_t *float_block;
      float_block = AudioStream_F32::receiveReadOnly_f32(); //float data block
      if (!float_block) return;

      //allocate a Int16 block
      audio_block_t *int_block;
      int_block = AudioStream::allocate(); 
      if (int_block == NULL) return;
      
      //convert back to int16
      convertAudio_F32ToI16(float_block, int_block, AUDIO_BLOCK_SAMPLES);

      //return audio to the system
      AudioStream::transmit(int_block);
      AudioStream::release(int_block);
      AudioStream_F32::release(float_block);
    };
    
  private:
    audio_block_f32_t *inputQueueArray_Float[1];
    static void convertAudio_F32ToI16(audio_block_f32_t *in, audio_block_t *out, int len) {
      const float MAX_INT = 32678.0;
      for (int i = 0; i < len; i++) {
        out->data[i] = (int16_t)(max(min( (in->data[i] * MAX_INT), MAX_INT), -MAX_INT));
      }
    }
};


