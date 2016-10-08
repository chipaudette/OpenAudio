
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
//define FFT_DATA_TYPE 16
#define FFT_DATA_TYPE 32
#if AUDIO_BLOCK_SAMPLES == 128
#define N_BUFF_BLOCKS (2)
#elif AUDIO_BLOCK_SAMPLES == 64
#define N_BUFF_BLOCKS (4)
#endif
typedef struct complex_t {
  #if FFT_DATA_TYPE == 16
    q15_t re  __attribute__ ((aligned(2)));
    q15_t im  __attribute__ ((aligned(2)));
  #elif FFT_DATA_TYPE == 32
    q31_t re  __attribute__ ((aligned(4)));
    q31_t im  __attribute__ ((aligned(4)));
  #endif
} complex_t;
class AudioEffectFreqDomain : public AudioStream
{
  public:
    AudioEffectFreqDomain(void) : AudioStream(1, inputQueueArray), window(AudioWindowHanning256), freqShift_bins(0) {
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
    void setFreqShiftBins(int value) {
      freqShift_bins = min(max(0,value),N_POS_BINS);
    }

  private:
    int freqShift_bins;
    audio_block_t *inputQueueArray[1];
    audio_block_t *input_buff_blocks[N_BUFF_BLOCKS];
    audio_block_t *output_buff_blocks[N_BUFF_BLOCKS];
    const int16_t *window;

    //q31_t buffer[N_FFT] __attribute__ ((aligned (4)));
    //q31_t second_buffer[N_FFT] __attribute__ ((aligned(4)));
    complex_t buffer[N_FFT];
    complex_t second_buffer[N_FFT];
    #if FFT_DATA_TYPE == 16
      arm_cfft_radix4_instance_q15 fft_inst;
      arm_cfft_radix4_instance_q15 ifft_inst;
    #elif FFT_DATA_TYPE == 32
      arm_cfft_radix4_instance_q31 fft_inst;
      arm_cfft_radix4_instance_q31 ifft_inst;
    #endif

    void clear_audio_block(audio_block_t *block) {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = 0;
    }
};

static void apply_window_to_fft_q15_buffer(complex_t buff[], int16_t win[])
{
  for (int i = 0; i < N_FFT; i++) {
    //int32_t val = buff[i].re * win[i];
    int32_t val = ((int32_t)buff[i].re) * ((int32_t)win[i]);
    //*buf = signed_saturate_rshift(val, 16, 15);
    buff[i].re = val >> 15;
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
  targ_ind = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
      #if FFT_DATA_TYPE == 16
        buffer[targ_ind].re = input_buff_blocks[i]->data[j];  //real
      #elif FFT_DATA_TYPE == 32
        buffer[targ_ind].re = ((q31_t)(input_buff_blocks[i]->data[j]));  //the 16-bit shift will occur naturally in the windowing
      #endif
      buffer[targ_ind].im = 0;  //imaginary
      targ_ind++;
    }
  }

  //apply window to the data
  if (window) {
    #if FFT_DATA_TYPE == 16
      apply_window_to_fft_q15_buffer(buffer, window);
    #else
      for (source_ind = 0; source_ind < N_FFT; source_ind++) {
        buffer[source_ind].re = buffer[source_ind].re * window[source_ind];
      }
    #endif
  } else {
    #if FFT_DATA_TYPE == 32
      for (source_ind = 0; source_ind < N_FFT; source_ind++) {
        buffer[source_ind].re = buffer[source_ind].re * 32767;  //max window value is 32767, which is same as a 15 bit shift
      }
    #endif
  }
  

  //call the FFT
  #if FFT_DATA_TYPE == 16
    arm_cfft_radix4_q15(&fft_inst, (q15_t *)buffer);
  #elif FFT_DATA_TYPE == 32
    arm_cfft_radix4_q31(&fft_inst, (q31_t *)buffer);
  #endif

  
  //Simplest.  Copy input buffer to output buffer.  No change to audio
  //for (source_ind=0; source_ind < N_FFT; source_ind++) second_buffer[source_ind] = buffer[source_ind]; //copies both real and imaginary parts

  //More complex.  Let's manipulate the data in the frequency domain
  //define SHIFT_BINS 1
  if (freqShift_bins > 0) {
    //for (targ_ind = 0; targ_ind < SHIFT_BINS; targ_ind++) { //clear the early bins. The rest will be overwritten
    for (targ_ind = 0; targ_ind < N_POS_BINS; targ_ind++) { //clear all of the bins.
      second_buffer[targ_ind].re = 0; //real
      second_buffer[targ_ind].im = 0; //imaginary
    }
  }  
  for (source_ind = 0; source_ind < N_POS_BINS; source_ind++) {
    targ_ind = source_ind + freqShift_bins;
    if ((targ_ind > 0) && (targ_ind < N_POS_BINS-1)) { //stay off DC and off Nyquist
      second_buffer[targ_ind].re=buffer[source_ind].re;  //copy both the real and imaginary parts (remove the divide by 4!!!)
      second_buffer[targ_ind].im=buffer[source_ind].im;  //copy both the real and imaginary parts (remove the divide by 4!!!)
    }
  }


  //create the negative frequency space via complex conjugate of the positive frequency space
  int ind_nyquist_bin = N_POS_BINS-1;
  targ_ind = ind_nyquist_bin+1;
  for (source_ind = ind_nyquist_bin-1; source_ind > 0; source_ind--) {
    second_buffer[targ_ind].re = second_buffer[source_ind].re; //real
    second_buffer[targ_ind].im = -second_buffer[source_ind].im; //imaginary.  negative makes it the complex conjugate, which is what we want for the neg freq space
    targ_ind++;
  }

  //call the IFFT
  #if FFT_DATA_TYPE == 16
    arm_cfft_radix4_q15(&ifft_inst, (q15_t *)second_buffer);
  #elif FFT_DATA_TYPE == 32
    arm_cfft_radix4_q31(&ifft_inst, (q31_t *)second_buffer);
  #endif

//   //window again
//  //if (window) {
//    #if FFT_DATA_TYPE == 16
//  //    apply_window_to_fft_q15_buffer(second_buffer, window);
//    #else
//      for (source_ind = 0; source_ind < N_FFT; source_ind++) {
//        second_buffer[source_ind].re = (second_buffer[source_ind].re >> 16) * window[source_ind]; //shift by 16 so that multiplying by 16 keeps within q31
//      }
//    #endif
//  //}
  
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
        output_buff_blocks[i]->data[j] +=  second_buffer[output_count].re; //get just the real component
      #elif FFT_DATA_TYPE == 32
        output_buff_blocks[i]->data[j] +=  (q15_t)(second_buffer[output_count].re >> 15); //get the real and down shift to q15_t
      #endif
      output_count++;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
    }
  }

  //now write in the newest data into the last block, overwriting any garbage that might have existed there
  for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
    #if FFT_DATA_TYPE == 16
      output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  second_buffer[output_count].re; //overwrite with the newest data
    #elif FFT_DATA_TYPE == 32
      output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  (q15_t)(second_buffer[output_count].re >> (15-8)); //the 8 is to recover the 8bits lost through IFFt
    #endif
    output_count++;  //buffer is [real, imaginary, real, imaginary,...] and we only want the reals.  so inrement by 2.
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
