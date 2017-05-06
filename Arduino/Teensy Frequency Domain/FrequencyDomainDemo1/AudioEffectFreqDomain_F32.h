
#ifndef _AudioEffecFreqDomain_F32_h
#define _AudioEffectFreqDomain_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "MyFFT_F32.h"
//#include "utility/dspinst.h"  //copied from analyze_fft256.cpp.  Do we need this?

#define MAX_N_BUFF_BLOCKS 16  //some number larger than you'll want to use

class AudioEffectFreqDomain_F32 : public AudioStream_F32
{
  public:
    //constructors
    AudioEffectFreqDomain_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectFreqDomain_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { 
      //int _N_FFT = 2*settings.audio_block_samples;  //default to 50% overlap!
      //setup(settings,_N_FFT);
    }
    AudioEffectFreqDomain_F32(const AudioSettings_F32 &settings, const int _N_FFT) : AudioStream_F32(1, inputQueueArray_f32) { 
      setup(settings,_N_FFT);
    }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectFreqDomain_F32(void) {
      for (int i = 0; i < N_BUFF_BLOCKS; i++) {
        if (input_buff_blocks[i] != NULL) AudioStream_F32::release(input_buff_blocks[i]);
        if (output_buff_blocks[i] != NULL) AudioStream_F32::release(output_buff_blocks[i]);
      }
      delete second_complex_buffer;
      delete complex_buffer;
    }
       
    void setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      //choose valid _N_FFT
      if (!myFFT.is_valid_N_FFT(_N_FFT)) {
          Serial.println(F("AudioEffectFreqDomain_F32: *** ERROR ***"));
          Serial.print(F("  : N_FFT ")); Serial.print(_N_FFT); 
          Serial.print(F(" is not allowed.  Try a power of 2 between 16 and 2048"));
          return;
      }
      
      //how many buffers will compose each FFT
      audio_block_samples = settings.audio_block_samples;
      N_BUFF_BLOCKS = _N_FFT / audio_block_samples; //truncates!
      N_BUFF_BLOCKS = max(1,min(4,N_BUFF_BLOCKS));

      //what does the fft length actually end up being
      N_FFT = N_BUFF_BLOCKS * audio_block_samples;
      
      //print FFT parameters
      Serial.println(F("AudioEffectFreqDomain: setup..."));
      Serial.print(F("    : N_FFT = ")); Serial.println(N_FFT);
      Serial.print(F("    : N_BUFF_BLOCKS = ")); Serial.println(N_BUFF_BLOCKS);

      //setup the FFT routines
      myFFT.setup(N_FFT); myFFT.useHanningWindow();
      myIFFT.setup(N_FFT); myIFFT.useRectangularWindow();

      //allocate memory for buffers
      complex_buffer = new float32_t[2*N_FFT];
      second_complex_buffer = new float32_t[2*N_FFT];

      //initialize the blocks for holding the previous data
      for (int i = 0; i < N_BUFF_BLOCKS; i++) {
        input_buff_blocks[i] = AudioStream_F32::allocate_f32();
        clear_audio_block(input_buff_blocks[i]);
        output_buff_blocks[i] = AudioStream_F32::allocate_f32();
        clear_audio_block(output_buff_blocks[i]);
      }
      enabled=1;
    }
    
    virtual void update(void);

//    void setFreqShiftBins(int value) {
//      freqShift_bins = min(max(0,value),N_POS_BINS);
//    }

  private:
    int N_BUFF_BLOCKS;
    int audio_block_samples;
    int N_FFT;
    int enabled=0;
    
    //int freqShift_bins;
    // //int printFFTtoSerial = 4;  //how many FFT results to print before stopping
    //int flipSignOddBins = 0;
    audio_block_f32_t *inputQueueArray_f32[1];
    audio_block_f32_t *input_buff_blocks[MAX_N_BUFF_BLOCKS];
    audio_block_f32_t *output_buff_blocks[MAX_N_BUFF_BLOCKS];

    float32_t *complex_buffer;
    float32_t *second_complex_buffer;
    MyFFT_F32 myFFT;
    MyIFFT_F32 myIFFT;

    void clear_audio_block(audio_block_f32_t *block) {
      for (int i = 0; i < block->length; i++) block->data[i] = 0.f;
    }
    
};


//
//static int adjustPhaseOfBins( complex_t second_complex_buffer[], const int freqShift_bins, const int n_overlap, int call_count) {
//  fft_data_t foo_val;
//  switch (n_overlap) {
//    case 1:   //0% overlap.  No phase shifting needed.
//      break;
//      
//    case 2:  //50% overlap.  Every other bin gets shifted by 180 deg on every other call
//      if (call_count==1) {
//        if ((freqShift_bins % 2) == 1) {
//          for (int i=0; i < N_POS_BINS; i++) {
//            second_complex_buffer[i].re = -second_complex_buffer[i].re;  //shift by 180 deg
//            second_complex_buffer[i].im = -second_complex_buffer[i].im ;    //shift by 180 deg
//          }
//        }
//      }
//      call_count = !call_count;
//      break;
//      
//   case 4:     //75% overlap.  Four call cycle
//    int phase_shift_pi_2 = (freqShift_bins*call_count) % 4;  //multiples of pi/2
//    switch (phase_shift_pi_2) {
//      case 0:
//        //no phase correction needed
//        break;    
//      case 1:
//        for (int i=0; i < N_POS_BINS; i++) {
//          foo_val = second_complex_buffer[i].re;
//          second_complex_buffer[i].re = -second_complex_buffer[i].im;  //shift by 90 deg
//          second_complex_buffer[i].im = foo_val;    //shift by 90 deg
//        }     
//        break;
//      case 2:
//        for (int i=0; i < N_POS_BINS; i++) {
//          second_complex_buffer[i].re = -second_complex_buffer[i].re;  //shift by 180 deg
//          second_complex_buffer[i].im = -second_complex_buffer[i].im ;    //shift by 180 deg
//        }
//        break;
//      case 3:
//        for (int i=0; i < N_POS_BINS; i++) {
//          foo_val = second_complex_buffer[i].re;
//          second_complex_buffer[i].re = second_complex_buffer[i].im;  //shift by 270 deg
//          second_complex_buffer[i].im = -foo_val;    //shift by 270 deg    
//        }
//        break;
//    }
//    call_count++;  if (call_count == 4) call_count=0;
//  }
//  return call_count;
//}
//
//static int shiftByIntegerNumberOfBins(complex_t complex_buffer[], complex_t second_complex_buffer[], const int freqShift_bins, const int n_overlap, int call_count) {
//  //shift the bins
//  int targ_ind = 0;
//  for (int source_ind = 0; source_ind < N_POS_BINS; source_ind++) {
//    targ_ind = source_ind + freqShift_bins;
//    if ((targ_ind > -1) && (targ_ind < N_POS_BINS)) {
//      second_complex_buffer[targ_ind].re = complex_buffer[source_ind].re;  //copy both the real and imaginary parts
//      second_complex_buffer[targ_ind].im = complex_buffer[source_ind].im;  
//    }
//  }
//
//  //adjust the phase of the bins
//  call_count = adjustPhaseOfBins(second_complex_buffer, freqShift_bins, n_overlap, call_count);
//  
//  return call_count;
//}

void AudioEffectFreqDomain_F32::update(void)
{
  int targ_ind, source_ind;

  //get a pointer to the latest data
  audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
  if (!block) return;

  //simply return the audio if this class hasn't been enabled
  if (!enabled) { AudioStream_F32::transmit(block); AudioStream_F32::release(block); return; }

  //shuffle all of input data blocks in preperation for this latest processing
  AudioStream_F32::release(input_buff_blocks[0]);  //release the oldest one
  for (int i = 1; i < N_BUFF_BLOCKS; i++) input_buff_blocks[i - 1] = input_buff_blocks[i];
  input_buff_blocks[N_BUFF_BLOCKS - 1] = block; //append the newest input data to the complex_buffer blocks

  //copy all input data blocks into one big block...the big block is interleaved [real,imaginary]
  targ_ind = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    for (int j = 0; j < audio_block_samples; j++) {
      complex_buffer[2*targ_ind] = input_buff_blocks[i]->data[j];  //real
      complex_buffer[2*targ_ind+1] = 0;  //imaginary
      targ_ind++;
    }
  }

  //windowing of the data happens in the FFT routine

  //call the FFT
  myFFT.execute(complex_buffer);

//  //Clear the target bins.  Is this necessary in all cases?
//  for (targ_ind = 0; targ_ind < 2*N_POS_BINS; targ_ind++) {
//    second_complex_buffer[targ_ind] = 0;
//  }

//  //apply the frequency domain processing algorithm that you care about
  switch (0) {
    case 0:
      //Simplest.  Copy all of the input complex_buffer to output complex_buffer.  No change to audio
      for (source_ind=0; source_ind < 2*N_FFT; source_ind++) second_complex_buffer[source_ind] = complex_buffer[source_ind]; //copies both real and imaginary parts
      break;
//    case 1:
//      //Do a frequency shift by a fixed number of bins
//      flipSignOddBins = shiftByIntegerNumberOfBins(complex_buffer, second_complex_buffer, freqShift_bins, N_BUFF_BLOCKS, flipSignOddBins);  
//      break;
//    case 10:
//      //Pure synthesis
//      second_complex_buffer[freqShift_bins].re = 33552631;
//      second_complex_buffer[freqShift_bins].im = 0;
//      if (flipSignOddBins) {
//        if ((freqShift_bins % 2) == 1) {  //only for 50% overlap
//          second_complex_buffer[freqShift_bins].re = -1*second_complex_buffer[freqShift_bins].re;
//          second_complex_buffer[freqShift_bins].im = -1*second_complex_buffer[freqShift_bins].im;
//        }
//      }
//      flipSignOddBins = !flipSignOddBins;
//      break;
  }

//  //create the negative frequency space via complex conjugate of the positive frequency space
//  int ind_nyquist_bin = N_POS_BINS-1;
//  targ_ind = ind_nyquist_bin+1;
//  for (source_ind = ind_nyquist_bin-1; source_ind > 0; source_ind--) {
//    second_complex_buffer[targ_ind].re = second_complex_buffer[source_ind].re; //real
//    second_complex_buffer[targ_ind].im = -second_complex_buffer[source_ind].im; //imaginary.  negative makes it the complex conjugate, which is what we want for the neg freq space
//    targ_ind++;
//  }

//   if (printFFTtoSerial) {
//    int printThisOne=0;
//    for (int i=0; i < N_FFT; i++) {
//      if (abs(complex_buffer[i].re) > 0) {
//        printThisOne = 1;
//      }
//    }
//    if (printThisOne > 0) {
//      Serial.print("FFT: ");
//      Serial.print(printFFTtoSerial);
//      Serial.print(".  Shifted ");
//      Serial.print(freqShift_bins);
//      Serial.println();
//      for (int i=0; i < N_FFT; i++) {
//        Serial.print(i);
//        Serial.print(": ");
//        Serial.print(complex_buffer[i].re);
//        Serial.print(", ");
//        Serial.print(complex_buffer[i].im);
//        Serial.print(", ");
//        Serial.print(second_complex_buffer[i].re);
//        Serial.print(", ");
//        Serial.print(second_complex_buffer[i].im);
//        Serial.println();
//      }
//      printFFTtoSerial--;
//    }
//  }
  

  //call the IFFT
  myIFFT.execute(second_complex_buffer);

  //any follow-up windowing is handdled in the IFFT routine
  
  //prepare for the overlap-and-add for the output
  audio_block_f32_t *temp_buff = output_buff_blocks[0]; //hold onto this one for a moment
  for (int i = 1; i < N_BUFF_BLOCKS; i++) output_buff_blocks[i - 1] = output_buff_blocks[i]; //shuffle the output data blocks
  output_buff_blocks[N_BUFF_BLOCKS - 1] = temp_buff; //put the oldest output buffer back in the list
  //clear_audio_block(output_buff_blocks[N_BUFF_BLOCKS-1]); //not needed because we'll simply overwrite with the newest data

  //do overlap and add with previously computed data
  int output_count = 0;
  for (int i = 0; i < N_BUFF_BLOCKS - 1; i++) { //Notice that this loop does NOT do the last block.  That's a special case after.
    for (int j = 0; j < audio_block_samples; j++) {
      output_buff_blocks[i]->data[j] +=  second_complex_buffer[2*output_count]; //add the real part into the previous results
      output_count++;
    }
  }

  //now write in the newest data into the last block, overwriting any garbage that might have existed there
  for (int j = 0; j < audio_block_samples; j++) {
    output_buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  second_complex_buffer[2*output_count]; //overwrite with the newest data
    output_count++;
  }

  //send the oldest data.  Don't issue the release command here because we will release it the next time through this routine
  transmit(output_buff_blocks[0]); //don't release this buffer because we re-use it every time this is called
  //transmit(input_buff_blocks[0]); //don't release this buffer because we re-use it every time this is called
  return;
};
#endif
