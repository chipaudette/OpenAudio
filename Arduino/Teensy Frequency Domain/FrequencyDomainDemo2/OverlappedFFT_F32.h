
#ifndef _OverlappedFFT_F32_h
#define _OverlappedFFT_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "MyFFT_F32.h"
//#include "utility/dspinst.h"  //copied from analyze_fft256.cpp.  Do we need this?

#define MAX_N_BUFF_BLOCKS 32  //some number larger than you'll want to use

class OverlappedFFT_Base_F32 {
  public:
    OverlappedFFT_Base_F32(void) {};
    ~OverlappedFFT_Base_F32(void) {
      if (N_BUFF_BLOCKS > 0) {
        for (int i = 0; i < N_BUFF_BLOCKS; i++) {
          if (buff_blocks[i] != NULL) AudioStream_F32::release(buff_blocks[i]);
        }
      }
      if (complex_buffer != NULL) delete complex_buffer;
    }

    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      int N_FFT;
      
      ///choose valid _N_FFT
      if (!MyFFT_F32::is_valid_N_FFT(_N_FFT)) {
          Serial.println(F("OverlappedFFT_Base: *** ERROR ***"));
          Serial.print(F("  : N_FFT ")); Serial.print(_N_FFT); 
          Serial.print(F(" is not allowed.  Try a power of 2 between 16 and 2048"));
          N_FFT = -1;
          return N_FFT;
      }
      
      //how many buffers will compose each FFT
      audio_block_samples = settings.audio_block_samples;
      N_BUFF_BLOCKS = _N_FFT / audio_block_samples; //truncates!
      N_BUFF_BLOCKS = max(1,min(MAX_N_BUFF_BLOCKS,N_BUFF_BLOCKS));

      //what does the fft length actually end up being
      N_FFT = N_BUFF_BLOCKS * audio_block_samples;

      //allocate memory for buffers
      complex_buffer = new float32_t[2*N_FFT];

      //initialize the blocks for holding the previous data
      for (int i = 0; i < N_BUFF_BLOCKS; i++) {
        buff_blocks[i] = AudioStream_F32::allocate_f32();
        clear_audio_block(buff_blocks[i]);
      }
      
      return N_FFT;
    }
    virtual int getNFFT(void) = 0;
    virtual int getNBuffBlocks(void) { return N_BUFF_BLOCKS; }

  protected:
    int N_BUFF_BLOCKS = 0;
    int audio_block_samples;
    
    audio_block_f32_t *buff_blocks[MAX_N_BUFF_BLOCKS];
    float32_t *complex_buffer;

    void clear_audio_block(audio_block_f32_t *block) {
      for (int i = 0; i < block->length; i++) block->data[i] = 0.f;
    }  
};

class OverlappedFFT_F32: public OverlappedFFT_Base_F32
{
  public:
    //constructors
    OverlappedFFT_F32(void): OverlappedFFT_Base_F32() {};
    OverlappedFFT_F32(const AudioSettings_F32 &settings): OverlappedFFT_Base_F32()  { }
    OverlappedFFT_F32(const AudioSettings_F32 &settings, const int _N_FFT): OverlappedFFT_Base_F32()  { 
      setup(settings,_N_FFT);
    }
       
    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      int N_FFT = OverlappedFFT_Base_F32::setup(settings, _N_FFT);
     
      //setup the FFT routines
      N_FFT = myFFT.setup(N_FFT); 
      return N_FFT;
    }
    
    virtual void execute(audio_block_f32_t *block, float *complex_2N_buffer);
    virtual int getNFFT(void) { return myFFT.getNFFT(); };
    virtual MyFFT_F32 getFFTObject(void) { return myFFT; };
  private:
    MyFFT_F32 myFFT;
};


class OverlappedIFFT_F32: public OverlappedFFT_Base_F32
{
  public:
    //constructors
    OverlappedIFFT_F32(void): OverlappedFFT_Base_F32() {};
    OverlappedIFFT_F32(const AudioSettings_F32 &settings): OverlappedFFT_Base_F32()  { }
    OverlappedIFFT_F32(const AudioSettings_F32 &settings, const int _N_FFT): OverlappedFFT_Base_F32()  { 
      setup(settings,_N_FFT);
    }
       
    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      int N_FFT = OverlappedFFT_Base_F32::setup(settings, _N_FFT);
     
      //setup the FFT routines
      N_FFT = myIFFT.setup(N_FFT); 
      return N_FFT;
    }
    
    virtual audio_block_f32_t* execute(float *complex_2N_buffer);
    virtual int getNFFT(void) { return myIFFT.getNFFT(); };
    virtual MyFFT_F32 getFFTObject(void) { return myIFFT; };
  private:
    MyIFFT_F32 myIFFT;
};



void OverlappedFFT_F32::execute(audio_block_f32_t *block, float *complex_2N_buffer)
{
  int targ_ind;

  //get a pointer to the latest data
  //audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
  if (!block) return;

  //add a claim to this block.  As a result, be sure that this function issues a "release()".
  //Also, be sure that the calling function issues its own release() to release its claim.
  block->ref_count++;  
  
  //shuffle all of input data blocks in preperation for this latest processing
  AudioStream_F32::release(buff_blocks[0]);  //release the oldest one
  for (int i = 1; i < N_BUFF_BLOCKS; i++) buff_blocks[i - 1] = buff_blocks[i];
  buff_blocks[N_BUFF_BLOCKS - 1] = block; //append the newest input data to the complex_buffer blocks

  //copy all input data blocks into one big block...the big block is interleaved [real,imaginary]
  targ_ind = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    for (int j = 0; j < audio_block_samples; j++) {
      complex_2N_buffer[2*targ_ind] = buff_blocks[i]->data[j];  //real
      complex_2N_buffer[2*targ_ind+1] = 0;  //imaginary
      targ_ind++;
    }
  }
  //call the FFT...windowing of the data happens in the FFT routine, if configured
  myFFT.execute(complex_2N_buffer);
}

audio_block_f32_t* OverlappedIFFT_F32::execute(float *complex_2N_buffer) {

//  //create the negative frequency space via complex conjugate of the positive frequency space
//  int ind_nyquist_bin = N_POS_BINS-1;
//  targ_ind = ind_nyquist_bin+1;
//  for (source_ind = ind_nyquist_bin-1; source_ind > 0; source_ind--) {
//    second_complex_buffer[targ_ind].re = second_complex_buffer[source_ind].re; //real
//    second_complex_buffer[targ_ind].im = -second_complex_buffer[source_ind].im; //imaginary.  negative makes it the complex conjugate, which is what we want for the neg freq space
//    targ_ind++;
//  }

  //call the IFFT...any follow-up windowing is handdled in the IFFT routine, if configured
  myIFFT.execute(complex_2N_buffer);

  
  //prepare for the overlap-and-add for the output
  audio_block_f32_t *temp_buff = buff_blocks[0]; //hold onto this one for a moment
  for (int i = 1; i < N_BUFF_BLOCKS; i++) buff_blocks[i - 1] = buff_blocks[i]; //shuffle the output data blocks
  buff_blocks[N_BUFF_BLOCKS - 1] = temp_buff; //put the oldest output buffer back in the list

  //do overlap and add with previously computed data
  int output_count = 0;
  for (int i = 0; i < N_BUFF_BLOCKS - 1; i++) { //Notice that this loop does NOT do the last block.  That's a special case after.
    for (int j = 0; j < audio_block_samples; j++) {
      buff_blocks[i]->data[j] +=  complex_2N_buffer[2*output_count]; //add the real part into the previous results
      output_count++;
    }
  }

  //now write in the newest data into the last block, overwriting any garbage that might have existed there
  for (int j = 0; j < audio_block_samples; j++) {
    buff_blocks[N_BUFF_BLOCKS - 1]->data[j] =  complex_2N_buffer[2*output_count]; //overwrite with the newest data
    output_count++;
  }

  //send the oldest data.  Don't issue the release command here because we will release it the next time through this routine
  //transmit(buff_blocks[0]); //don't release this buffer because we re-use it every time this is called
  return buff_blocks[0]; //send back the pointer to this audio block...but don't release it because we'll re-use it here
};
#endif
