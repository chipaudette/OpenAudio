
/*
   AudioEffectFeedbackCancel_F32

   Created: Chip Audette, OpenAudio May 2018
   Purpose: Adaptive feedback cancelation.  Algorithm from Boys Town National Research Hospital
       BTNRH at: https://github.com/BoysTownorg/chapro

   This processes a single stream fo audio data (ie, it is mono)

   MIT License.  use at your own risk.
*/

#ifndef _AudioEffectFeedbackCancel_F32
#define _AudioEffectFeedbackCancel_F32

#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>
#include <BTNRH_WDRC_Types.h> //from Tympan_Library
#include <Arduino.h>  //for Serial.println()

#ifndef MAX_AFC_FILT_LEN
#define MAX_AFC_FILT_LEN  256
#endif

class AudioEffectFeedbackCancel_F32 : public AudioStream_F32
{
    //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
    //GUI: shortName: FB_Cancel
  public:
    //constructor
    AudioEffectFeedbackCancel_F32(void) : AudioStream_F32(1, inputQueueArray_f32){
      setDefaultValues();
      initializeStates();
      initializeRingBuffer();
    }
    AudioEffectFeedbackCancel_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {
      //do any setup activities here
      setDefaultValues();
      initializeStates();
      initializeRingBuffer();

      //if you need the sample rate, it is: fs_Hz = settings.sample_rate_Hz;
      //if you need the block size, it is: n = settings.audio_block_samples;
    };

    void setDefaultValues(void) {
      //set default values...taken from BTNRH tst_iffb.c
      float _mu = 1.E-3;
      float _rho = 0.984;
      int _afl = 100; //for 24kHz sample rate
      int _sqm = 1;
      float _fbg = 1.0;
      int _ds = 200;
      float _gs = 4.0;
      setParams(_mu, _rho, _afl, _sqm, _fbg, _ds, _gs);
    }

    void setParams(float _mu, float _rho, int _afl, int _sqm, float _fbg, int _ds, float _gs) {
      //AFC parameters
      mu = _mu;     // AFC step size
      rho = _rho;   // AFC forgetting factor
      afl  = _afl;  // AFC adaptive filter length
      if (afl > max_afc_ringbuff_len) {
        Serial.println(F("AudioEffectFeedbackCancel_F32: *** ERROR ***"));
        Serial.print(F("    : Adaptive filter length (")); Serial.print(afl);
        Serial.print(F(") too long for ring buffer (")); Serial.print(max_afc_ringbuff_len); Serial.println(")");
        afl = max_afc_ringbuff_len;
        Serial.print(F("    : Limiting filter length to ")); Serial.println(afl);
      }
      sqm = _sqm;   // AFC save quality metric ?
      nqm = (fbl < afl) ? fbl : afl;  //number of quality metrics?

      //parameters for simulating a feedback path
      fbg = _fbg;   // simulated feedback gain
      ds = _ds;     // simulated processing delay
      gs = _gs;     // simulated processing gain
      initializeSimulatedFeedbackResponse();

    }
    void setEnable(bool _enable) { enable = _enable; };

    //ring buffer
    static const int max_afc_ringbuff_len = MAX_AFC_FILT_LEN;
    float32_t ring[max_afc_ringbuff_len];
    int rhd, rtl;
    void initializeRingBuffer(void) {
      rhd = 0;
      rtl = 0;
      for (int i = 0; i < max_afc_ringbuff_len; i++) ring[i] = 0.0;
    }
    int rsz = max_afc_ringbuff_len;  //"ring buffer size"...variable name inherited from original BTNRH code
    int mask = rsz - 1;

    //initializeStates
    void initializeStates(void) {
      pwr = 0.0;
      for (int i = 0; i < MAX_AFC_FILT_LEN; i++) efbp[i] = 0.0;
    }

    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
     
      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;

      //allocate memory for the output of our algorithm
      audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
      if (!out_block) {
        AudioStream_F32::release(in_block);
        return;
      }

      //do the work
      if (enable) {
        cha_afc_input(in_block->data, out_block->data, in_block->length);
      } else {
        //simply copy input to output
        for (int i=0; i < in_block->length; i++) out_block->data[i] = in_block->data[i];
      }

      // transmit the block and release memory
      AudioStream_F32::transmit(out_block); // send the FIR output
      AudioStream_F32::release(out_block);
      AudioStream_F32::release(in_block);
    }

    //void cha_afc_input(CHA_PTR cp, float *x, float *y, int cs) //called from process_chunk.  Step 1 of 4 for AFC processing
    void cha_afc_input(float *x, //input audio block
                       float *y, //output audio block
                       int cs)  //"chunk size"...ie, the length of the audio block
    {
      //float *ring, *efbp, *sfbp, *merr;
      //float fbe, fbs, mum, dif, fbm, dm, s0, s1, mu, rho, pwr, ipwr, eps = 1e-30f;
      //int i, ii, ij, j, afl, fbl, nqm, rsz, rhd, mask;
      float fbe, fbs, mum, dif, fbm, dm, s0, s1, ipwr, eps = 1e-30f;
      int i, ii, ij, j;

      // subtract estimated feedback signal
      for (i = 0; i < cs; i++) {  //step through WAV sample-by-sample
        s0 = x[i];  //current waveform sample
        ii = rhd + i;

        // simulate feedback [don't need this in the real-time system?]
        fbs = 0;
#if 0
        for (j = 0; j < fbl; j++) {
          ij = (ii - j + rsz) & mask;
          fbs += ring[ij] * sfbp[j];  //feedback signal
        }
#endif

        // estimate feedback
        fbe = 0;
        for (j = 0; j < afl; j++) {
          ij = (ii - j + rsz) & mask;
          fbe += ring[ij] * efbp[j];
        }

        // apply feedback to input signal
        //s1 = s0 + fbs - fbe;
        s1 = s0 - fbe;

        // calculate instantaneous power
        ipwr = s0 * s0 + s1 * s1;

        // update adaptive feedback coefficients
        pwr = rho * pwr + ipwr;
        mum = mu / (eps + pwr);  // modified mu
        for (j = 0; j < afl; j++) {
          ij = (ii - j + rsz) & mask;
          efbp[j] += mum * ring[ij] * s1;  //update the estimated feedback coefficients
        }

        // save quality metrics
        #if 0
        if (nqm > 0) {
          dm = 0;
          for (j = 0; j < nqm; j++) {
            dif = sfbp[j] - efbp[j];
            dm += dif * dif;
          }
          merr[i] = dm / fbm;
        }
        #endif

        // copy AFC signal to output
        y[i] = s1;
      }
      // save power estimate
      //CHA_DVAR[_pwr] = pwr;
    }

    void updateRingBuffer(float *x, int cs) {
      cha_afc_output(x,cs);
    }

    //void cha_afc_output(CHA_PTR cp, float *x, int cs)  //called from process_chunk.  Step 4 of 4 of AFC processing
    void cha_afc_output(float *x,  //input audio block
                        int cs)    //number of samples in this audio block
    {
      //float *ring;
      //int i, j, rsz, rhd, rtl, mask;
      int i, j;

      //rsz = CHA_IVAR[_rsz];    //size of ring buffer
      //rhd = CHA_IVAR[_rhd];    //where to start in the target ring buffer.  Overwritten!
      //rtl = CHA_IVAR[_rtl];    //where it last ended in the target ring buffer
      //ring = (float *) cp[_ring];
      //mask = rsz - 1;          //what sample to jump back to zero?
      rhd = rtl;   //where to start in the target ring buffer
      // copy chunk to ring buffer
      for (i = 0; i < cs; i++) {  //loop over each sample in the audio chunk
        j = (rhd + i) & mask;  //get index into target array.  This a fast way of doing modulo?
        ring[j] = x[i];   //save sample into ring buffer
      }
      rtl = (rhd + cs) % rsz;  //save the last write point in the ring buffer
      //CHA_IVAR[_rhd] = rhd;   //save the start point
      //CHA_IVAR[_rtl] = rtl;   //save the end point
    }

  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enable = true;

    //AFC parameters
    float32_t mu;    // AFC step size
    float32_t rho;   // AFC forgetting factor
    int afl;         // AFC adaptive filter length

    //FDC states
    float32_t pwr;   // AFC estimate of error power...a state variable
    float32_t efbp[MAX_AFC_FILT_LEN];  //vector holding the estimated feedback impulse response

    //parameters for simulating a feedback path
    float32_t fbg;   // simulated feedback gain
    int ds;     // simulated processing delay
    float32_t gs;     // simulated processing gain

    // exampleITE feedback path from Kosgen (2006)
    static const int fbl = 100;  //length of the given feedback impulse response (for simulation purposes)
    const float32_t ite_fbp[fbl] = {  //given feedback inpulse response (for simulation purposes)
      0.000000, -0.001715, 0.000306, 0.007936, -0.014126, 0.001207, 0.001541, 0.040498, 0.077863, 0.069577,
      -0.008025, -0.106044, -0.151131, -0.124264, -0.055777, 0.013607, 0.075156, 0.082472, 0.048781, 0.004444,
      -0.033910, -0.033370, -0.013614, 0.011497, 0.031987, 0.040751, 0.031424, 0.014976, -0.001471, -0.006256,
      -0.008080, -0.003218, -0.000933, 0.001967, -0.000208, -0.003719, -0.009216, -0.012280, -0.014076, -0.013594,
      -0.013487, -0.010067, -0.007326, -0.004848, -0.003921, -0.003026, -0.002302, -0.001824, -0.001345, -0.000225,
      0.001573, 0.003371, 0.003689, 0.003543, 0.002901, 0.002056, 0.001375, 0.000886, 0.000398, -0.000091,
      -0.000580, -0.001069, -0.001569, -0.002153, -0.002737, -0.003321, -0.003905, -0.004489, -0.005073, -0.004653,
      -0.004184, -0.003715, -0.003246, -0.002778, -0.002309, -0.001965, -0.001722, -0.001479, -0.001236, -0.000993,
      -0.000750, -0.000819, -0.000969, -0.001118, -0.001268, -0.001418, -0.001567, -0.001717, -0.001867, -0.002016,
      -0.002166, -0.002200, -0.002200, -0.002200, -0.002200, -0.002200, -0.002200, -0.002200, -0.002200, -0.002200
    };
    float32_t sfbp[fbl];  //vector holding the feedback impuse repsonse to use when simulating feedback
    void initializeSimulatedFeedbackResponse(void) {
      fbm = 0.0;
      for (int i = 0; i < fbl; i++) {
        sfbp[i] = ite_fbp[i] * fbg;
        fbm += sfbp[i] * sfbp[i];
      }
    }

    //AFC quality metrics
    int sqm;   // AFC save quality metric ?
    float32_t fbm;  //feedback magnitude?  for error assessment?
    int nqm;   //number of quality metrics
    float32_t merr[fbl];  //allocate space to hold the quality metrics


};  //end class definition


class AudioEffectFeedbackCancel_LoopBack_F32 : public AudioStream_F32
{
    //GUI: inputs:1, outputs:0  //this line used for automatic generation of GUI node
    //GUI: shortName: FB_Cancel_LoopBack
  public:
    //constructor
    AudioEffectFeedbackCancel_LoopBack_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { }
    AudioEffectFeedbackCancel_LoopBack_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { };

    void setTargetAFC(AudioEffectFeedbackCancel_F32 *_afc) {
      AFC_obj = _afc;
    }

    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
      if (AFC_obj == NULL) return;

      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;

      //do the work
      AFC_obj->updateRingBuffer(in_block->data, in_block->length);

      //release memory
      AudioStream_F32::release(in_block);
    }

  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    AudioEffectFeedbackCancel_F32 *AFC_obj;
};

#endif
