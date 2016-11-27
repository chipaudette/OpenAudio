
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

class AudioStream_Float : public AudioStream {
  public:
    AudioStream_Float(unsigned char ninput_float, audio_float_block_t **iqueue) : AudioStream(1, inputQueue), 
        num_inputs_float(ninput_float), inputQueue_float(iqueue) {
      active = false;
      destination_list_float = NULL;
      for (int i=0; i < num_inputs; i++) {
        inputQueue_float[i] = NULL;
      }
    };
    static void initialize_memory_float(audio_float_block_t *data, unsigned int num);
    virtual void update(audio_float_block_t *) = 0; 
    static uint8_t float_memory_used;
    static uint8_t float_memory_used_max;
  protected:
    bool active_float;
    unsigned char num_inputs_float;
    static audio_float_block_t * allocate_float(void);
    static void release(audio_float_block_t * block);
    void transmit(audio_float_block_t *block, unsigned char index = 0);
    audio_float_block_t * receiveReadOnly_Float(unsigned int index = 0);
    audio_float_block_t * receiveWritable_Float(unsigned int index = 0);  
    friend class AudioConnection_Float;
  private:
    AudioConnection_Float *destination_list_float;
    audio_float_block_t **inputQueue_float;
    audio_block_t **inputQueue;
    static audio_float_block_t *memory_pool_float;
    static uint32_t memory_pool_float_available_mask[6];
};


audio_float_block_t * AudioStream_Float::memory_pool_float;
uint32_t AudioStream_Float::memory_pool_float_available_mask[6];

uint8_t AudioStream_Float::float_memory_used = 0;
uint8_t AudioStream_Float::float_memory_used_max = 0;

// Set up the pool of audio data blocks
// placing them all onto the free list
void AudioStream_Float::initialize_memory_float(audio_float_block_t *data, unsigned int num)
{
  unsigned int i;

  //Serial.println("AudioStream_Float initialize_memory");
  //delay(10);
  if (num > 192) num = 192;
  __disable_irq();
  memory_pool_float = data;
  for (i=0; i < 6; i++) {
    memory_pool_float_available_mask[i] = 0;
  }
  for (i=0; i < num; i++) {
    memory_pool_float_available_mask[i >> 5] |= (1 << (i & 0x1F));
  }
  for (i=0; i < num; i++) {
    data[i].memory_pool_index = i;
  }
  __enable_irq();

}



// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_float_block_t * AudioStream_Float::allocate_float(void)
{
  uint32_t n, index, avail;
  uint32_t *p;
  audio_float_block_t *block;
  uint8_t used;

  p = memory_pool_float_available_mask;
  __disable_irq();
  do {
    avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    __enable_irq();
    //Serial.println("alloc_float:null");
    return NULL;
  } while (0);
  n = __builtin_clz(avail);
  *p = avail & ~(0x80000000 >> n);
  used = float_memory_used + 1;
  float_memory_used = used;
  __enable_irq();
  index = p - memory_pool_float_available_mask;
  block = memory_pool_float + ((index << 5) + (31 - n));
  block->ref_count = 1;
  if (used > float_memory_used_max)float_memory_used_max = used;
  //Serial.print("alloc_float:");
  //Serial.println((uint32_t)block, HEX);
  return block;
}


// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream_Float::release(audio_float_block_t *block)
{
  uint32_t mask = (0x80000000 >> (31 - (block->memory_pool_index & 0x1F)));
  uint32_t index = block->memory_pool_index >> 5;


  __disable_irq();
  if (block->ref_count > 1) {
    block->ref_count--;
  } else {
    //Serial.print("release_float:");
    //Serial.println((uint32_t)block, HEX);
    memory_pool_float_available_mask[index] |= mask;
    float_memory_used--;
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
void AudioStream_Float::transmit(audio_float_block_t *block, unsigned char index)
{
  for (AudioConnection_Float *c = destination_list_float; c != NULL; c = c->next_dest) {
    if (c->src_index == index) {
      if (c->dst.inputQueue_float[c->dest_index] == NULL) {
        c->dst.inputQueue_float[c->dest_index] = block;
        block->ref_count++;
      }
    }
  }      
}

// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_float_block_t * AudioStream_Float::receiveReadOnly_Float(unsigned int index)
{
  audio_float_block_t *in;

  if (index >= num_inputs_float) return NULL;
  in = inputQueue_float[index];
  inputQueue_float[index] = NULL;
  return in;
}


// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_float_block_t * AudioStream_Float::receiveWritable_Float(unsigned int index)
{
  audio_float_block_t *in, *p;

  if (index >= num_inputs_float) return NULL;
  in = inputQueue_float[index];
  inputQueue_float[index] = NULL;
  if (in && in->ref_count > 1) {
    p = allocate_float();
    if (p) memcpy(p->data, in->data, sizeof(p->data));
    in->ref_count--;
    in = p;
  }
  return in;
}

void AudioConnection_Float::connect(void) {
  AudioConnection_Float *p;
  
  if (dest_index > dst.num_inputs_float) return;
  __disable_irq();
  p = src.destination_list_float;
  if (p == NULL) {
    src.destination_list_float = this;
  } else {
    while (p->next_dest) p = p->next_dest;
    p->next_dest = this;
  }
  src.active_float = true;
  dst.active_float = true;
  __enable_irq();
}


