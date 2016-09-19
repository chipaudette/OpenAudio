
#ifndef effect_freqDomain_h_
#define effect_freqDomain_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "utility/dspinst.h"  //copied from analyze_fft256.cpp.  Do we need this?

// windows.c
extern "C" {
  extern const int16_t AudioWindowHanning256[];
  extern const int16_t AudioWindowBartlett256[];
  extern const int16_t AudioWindowBlackman256[];
  extern const int16_t AudioWindowFlattop256[];
  extern const int16_t AudioWindowBlackmanHarris256[];
  extern const int16_t AudioWindowNuttall256[];
  extern const int16_t AudioWindowBlackmanNuttall256[];
  extern const int16_t AudioWindowWelch256[];
  extern const int16_t AudioWindowHamming256[];
  extern const int16_t AudioWindowCosine256[];
  extern const int16_t AudioWindowTukey256[];
}

//assumes ADUIO_BLOCK_SAMPLES is 64 or 128.  Assumes 50% overlap
#define N_FFT 256
#if AUDIO_BLOCK_SAMPLES == 128
#define N_BUFF_BLOCKS (2)
#elif AUDIO_BLOCK_SAMPLES == 64
#define N_BUFF_BLOCKS (4)
#endif
class AudioEffectFreqDomain : public AudioStream
{
  public:
    AudioEffectFreqDomain(void) : AudioStream(1, inputQueueArray), window(AudioWindowHanning256) {
      //initialize FFT and IFFT functions
      arm_cfft_radix4_init_q15(&fft_inst, N_FFT, 0, 1);
      arm_cfft_radix4_init_q15(&ifft_inst, N_FFT, 1, 1);

      //initialize the blocks for holding the previous data
      for (int i = 0; i < N_BUFF_BLOCKS; i++) {
        input_buff_blocks[i] = allocate();
        clear_audio_block(input_buff_blocks[i]);

        output_buff_blocks[i] = allocate();
        clear_audio_block(output_buff_blocks[i]);
      }
    }
    void windowFunction(const int16_t *w) {
      window = w;
    }
    virtual void update(void);

    ~AudioEffectFreqDomain(void) {
      //release allcoated memory
      for (int i = 0; i < N_BUFF_BLOCKS; i++) {
        release(input_buff_blocks[i]);
        release(output_buff_blocks[i]);
      }
    }

  private:
    audio_block_t *input_buff_blocks[N_BUFF_BLOCKS];
    audio_block_t *output_buff_blocks[N_BUFF_BLOCKS];
    int16_t buffer[2 * N_FFT] __attribute__ ((aligned (4)));
    const int16_t *window;

    audio_block_t *inputQueueArray[1];
    arm_cfft_radix4_instance_q15 fft_inst;
    arm_cfft_radix4_instance_q15 ifft_inst;
    void clear_audio_block(audio_block_t *block) {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = 0;
    }
};

// 140312 - PAH - slightly faster copy
static void copy_to_fft_buffer(void *destination, const void *source)
{
  const uint16_t *src = (const uint16_t *)source;
  uint32_t *dst = (uint32_t *)destination;

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    *dst++ = *src++;  // real sample plus a zero for imaginary
  }
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
  int16_t *buf = (int16_t *)buffer;
  const int16_t *win = (int16_t *)window;;

  for (int i = 0; i < N_FFT; i++) {
    int32_t val = *buf * *win++;
    //*buf = signed_saturate_rshift(val, 16, 15);
    *buf = val >> 15;
    buf += 2;
  }
}


void AudioEffectFreqDomain::update(void)
{
  //get a pointer to the latest data
  audio_block_t *block;
  block = receiveReadOnly();
  if (!block) {
    transmit(block);
    release(block);
    return;
  }

  //shuffle all of input data blocks in preperation for this latest processing
  release(input_buff_blocks[0]);  //release the oldest one
  for (int i = 1; i < N_BUFF_BLOCKS; i++) input_buff_blocks[i - 1] = input_buff_blocks[i];
  input_buff_blocks[N_BUFF_BLOCKS - 1] = block; //append the newest input data to the buffer blocks

  //copy all input data blocks into one big block
  int start_count = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    copy_to_fft_buffer(buffer + start_count, input_buff_blocks[i]->data);
    start_count += (2 * AUDIO_BLOCK_SAMPLES); //step size is 2*AUDIO_BLOCK_SAMPLES because "buffer" is for complex data (so 2 entries per Int16)
  }

  //apply window to the data
  if (window) apply_window_to_fft_buffer(buffer, window);

  //call the FFT
  arm_cfft_radix4_q15(&fft_inst, buffer);

  //Manipulate the data in the frequency domain
  //my_freq_domain_processing();  //do operations in "buffer"

  //call the IFFT
  arm_cfft_radix4_q15(&ifft_inst, buffer);

  //prepare for the overlap-and-add for the output
  audio_block_t *temp_buff = output_buff_blocks[0]; //hold onto this one for a moment
  for (int i = 1; i < N_BUFF_BLOCKS; i++) output_buff_blocks[i - 1] = output_buff_blocks[i]; //shuffle the output data blocks
  output_buff_blocks[N_BUFF_BLOCKS - 1] = temp_buff; //put the oldest output buffer back in the list
  //clear_audio_block(output_buff_blocks[N_BUFF_BLOCKS-1]); //not needed because we'll simply overwrite with the newest data

  //do overlap and add with previously computed data
  int output_count = 0;
  for (int i = 0; i < N_BUFF_BLOCKS - 1; i++) { //Notice that this loop does NOT do the last block.  That's a special case after.
    for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
      output_buff_blocks[i]->data[j] +=  buffer[output_count]; 
      output_count += 2;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
    }
  }

  //now write in the newest data into the last block, overwriting any garbage that might have existed there
  for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
    output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  buffer[output_count]; //overwrite with the newest data
    output_count += 2;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
  }

  //send the oldest data.  Don't issue the release command here because we will release it the next time through this routine
  transmit(output_buff_blocks[0]);
  return;
}

#endif
