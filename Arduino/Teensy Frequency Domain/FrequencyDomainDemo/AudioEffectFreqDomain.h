
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
#define N_POS_BINS (N_FFT/2+1)
//#define FFT_DATA_TYPE 16
#define FFT_DATA_TYPE 32
#if AUDIO_BLOCK_SAMPLES == 128
#define N_BUFF_BLOCKS (2)
#elif AUDIO_BLOCK_SAMPLES == 64
#define N_BUFF_BLOCKS (4)
#endif
class AudioEffectFreqDomain : public AudioStream
{
  public:
    AudioEffectFreqDomain(void) : AudioStream(1, inputQueueArray), window(NULL) {
    }
    void setup(void) {
      Serial.println("AudioEffectFreqDomain: setup...");
      Serial.print("    : N_FFT = "); Serial.println(N_FFT);
      Serial.print("    : N_BUFF_BLOCKS = "); Serial.println(N_BUFF_BLOCKS);

      //initialize FFT and IFFT functions
      #if FFT_DATA_TYPE == INT
        arm_cfft_radix4_init_q15(&fft_inst, N_FFT, 0, 1); //FFT
        arm_cfft_radix4_init_q15(&ifft_inst, N_FFT, 1, 1); //IFFT
      #elif FFT_DATA_TYPE == 32
        arm_cfft_radix4_init_q31(&fft_inst, N_FFT, 0, 1); //FFT
        arm_cfft_radix4_init_q31(&ifft_inst, N_FFT, 1, 1); //IFFT
      #endif

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
        if (input_buff_blocks[i] != NULL) release(input_buff_blocks[i]);
        if (output_buff_blocks[i] != NULL) release(output_buff_blocks[i]);
      }
    }

  private:
    audio_block_t *inputQueueArray[1];
    audio_block_t *input_buff_blocks[N_BUFF_BLOCKS];
    audio_block_t *output_buff_blocks[N_BUFF_BLOCKS];
    const int16_t *window;

    #if FFT_DATA_TYPE == 16
      arm_cfft_radix4_instance_q15 fft_inst;
      arm_cfft_radix4_instance_q15 ifft_inst;
      q15_t buffer[2 * N_FFT] __attribute__ ((aligned (4)));
      q15_t second_buffer[2 * N_FFT] __attribute__ ((aligned (4)));
    #elif FFT_DATA_TYPE == 32
      arm_cfft_radix4_instance_q31 fft_inst;
      arm_cfft_radix4_instance_q31 ifft_inst;
      q31_t buffer[2 * N_FFT] __attribute__ ((aligned (4)));
      q31_t second_buffer[2 * N_FFT] __attribute__ ((aligned (4)));
    #endif

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
  int targ_ind, source_ind;
  
  //get a pointer to the latest data
  audio_block_t *block;
  block = receiveReadOnly();
  if (!block) return;

  //shuffle all of input data blocks in preperation for this latest processing
  release(input_buff_blocks[0]);  //release the oldest one
  for (int i = 1; i < N_BUFF_BLOCKS; i++) input_buff_blocks[i - 1] = input_buff_blocks[i];
  input_buff_blocks[N_BUFF_BLOCKS - 1] = block; //append the newest input data to the buffer blocks

  //clear the buffer?
  //for (int i=0; i < 2*N_FFT; i++) buffer[i]=0;

  //copy all input data blocks into one big block
  //int start_count = 0;
  targ_ind = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    //copy_to_fft_buffer(buffer + start_count, input_buff_blocks[i]->data);
    //start_count += 2 * N_FFT;
    for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
      #if FFT_DATA_TYPE == 16
        buffer[targ_ind++] = input_buff_blocks[i]->data[j];  //real
      #elif FFT_DATA_TYPE == 32
        if (window) {
          buffer[targ_ind++] = ((q31_t)(input_buff_blocks[i]->data[j]));  //the 16-bit shift will occur naturally in the windowing
        } else {
          buffer[targ_ind++] = ((q31_t)(input_buff_blocks[i]->data[j])) << 16; 
        }
      #endif
      buffer[targ_ind++] = 0;  //imaginary
    }
  }

  //apply window to the data
  if (window) {
    #if FFT_DATA_TYPE == 16
      apply_window_to_fft_buffer(buffer, window);
    #else
      for (source_ind = 0; source_ind < N_FFT; source_ind++) buffer[2*source_ind] = buffer[2*source_ind]*window[source_ind];
    #endif
  }


  //call the FFT
  #if FFT_DATA_TYPE == 16
    arm_cfft_radix4_q15(&fft_inst, buffer);
  #elif FFT_DATA_TYPE == 32
    arm_cfft_radix4_q31(&fft_inst, buffer);
  #endif

  
//  //Simplest.  Copy input buffer to output buffer.
//  for (source_ind=0; source_ind < N_FFT; source_ind++) {
//    if (source_ind < N_POS_BINS) {
//      second_buffer[2*source_ind] = buffer[2*source_ind];
//      second_buffer[2*source_ind+1] = buffer[2*source_ind+1];
//    } else {
//      second_buffer[2*source_ind] = 0;
//      second_buffer[2*source_ind+1] = 0;
//    }
//  }

  //More complex.  Let's manipulate the data in the frequency domain
  #define SHIFT_BINS 0
  if (SHIFT_BINS > 0) {
    //for (targ_ind = 0; targ_ind < SHIFT_BINS; targ_ind++) { //clear the early bins. The rest will be overwritten
    for (targ_ind = 0; targ_ind < N_FFT; targ_ind++) { //clear the early bins. The rest will be overwritten
      second_buffer[2 * targ_ind] = 0; //real
      second_buffer[2 * targ_ind + 1] = 0; //imaginary
    }
  }  
  for (source_ind = 0+1; source_ind < N_POS_BINS; source_ind++) { //skip the DC bin
    targ_ind = source_ind + SHIFT_BINS;
    if ((targ_ind >= 0) && (targ_ind < N_POS_BINS)) {
      second_buffer[2 * targ_ind] = buffer[2 * source_ind]; //move everything up to higher frequency...real
      second_buffer[2 * targ_ind + 1] = buffer[2 * source_ind + 1]; //move everything up to higher frequency...imaginary
    }
  }


  //create the negative frequency space via complex conjugate of the positive frequency space
  int ind_nyquist_bin = N_POS_BINS-1;
  targ_ind = ind_nyquist_bin+1;
  for (source_ind = ind_nyquist_bin-1; source_ind > 0; source_ind--) {
    second_buffer[2 * targ_ind] = second_buffer[2 * source_ind]; //real
    second_buffer[2 * targ_ind + 1] = -second_buffer[2 * source_ind + 1]; //imaginary.  negative magkes it the complex conjugate
    targ_ind++;
  }

  //call the IFFT
  #if FFT_DATA_TYPE == 16
    arm_cfft_radix4_q15(&ifft_inst, second_buffer);
  #elif FFT_DATA_TYPE == 32
    arm_cfft_radix4_q31(&ifft_inst, second_buffer);
  #endif
  
  //prepare for the overlap-and-add for the output
  audio_block_t *temp_buff = output_buff_blocks[0]; //hold onto this one for a moment
  for (int i = 1; i < N_BUFF_BLOCKS; i++) output_buff_blocks[i - 1] = output_buff_blocks[i]; //shuffle the output data blocks
  output_buff_blocks[N_BUFF_BLOCKS - 1] = temp_buff; //put the oldest output buffer back in the list
  //clear_audio_block(output_buff_blocks[N_BUFF_BLOCKS-1]); //not needed because we'll simply overwrite with the newest data

  //do overlap and add with previously computed data
  int output_count = 0;
  for (int i = 0; i < N_BUFF_BLOCKS - 1; i++) { //Notice that this loop does NOT do the last block.  That's a special case after.
    for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
      #if FFT_DATA_TYPE == 16
        output_buff_blocks[i]->data[j] +=  second_buffer[output_count];
      #elif FFT_DATA_TYPE == 32
        output_buff_blocks[i]->data[j] +=  (q15_t)(second_buffer[output_count] >> 16);
      #endif
      output_count += 2;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
    }
  }

  //now write in the newest data into the last block, overwriting any garbage that might have existed there
  for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
    #if FFT_DATA_TYPE == 16
      output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  second_buffer[output_count]; //overwrite with the newest data
    #elif FFT_DATA_TYPE == 32
      output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  (q15_t)(second_buffer[output_count] >> (16-8)); //the 8 is to recover the 8bits lost through IFFt
    #endif
    output_count += 2;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
  }

  //scale the IFFT to make up for the Q15 IFFT gain loss (8 bits?)
  //const int bits_to_shift=8;
  //arm_scale_q15(output_buff_blocks[0],1,bits_to_shift,output_buff_blocks[0],AUDIO_BLOCK_SAMPLES);
  #if FFT_DATA_TYPE == 16
    for (int j=0; j < AUDIO_BLOCK_SAMPLES; j++) output_buff_blocks[0]->data[j] <<= 7;  //only needed to account for loss in Q15 IFFT?
  #endif

  //send the oldest data.  Don't issue the release command here because we will release it the next time through this routine
  transmit(output_buff_blocks[0]); //don't release this buffer because we re-use it every time this is called
  //transmit(input_buff_blocks[0]); //don't release this buffer because we re-use it every time this is called
  return;
};
#endif
