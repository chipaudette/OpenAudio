/*
 * AudioEffectExpCompLim_F32: Expander-Compressor-Limitter
 * 
 * Created: Chip Audette (OpenAudio) Mar 2017
 * Derived From: AudioEffectCompWDRCF32
 * 
 * Purpose: This is an expander algorithm.  For any signal level
 *   below the threshold, it cuts the gain applied to the signal
 *   based on the expansion_ratio.  Typically, this is used
 *   to attenuate the perceived noise of a signal as the raw
 *   signal gets quiet enough to reveal the noise floor 
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _AudioEffectExpCompLim_F32
#define _AudioEffectExpCompLim_F32

#include <Arduino.h>
#include <AudioStream_F32.h>
#include <arm_math.h>
#include <AudioCalcEnvelope_F32.h>
#include "AudioCalcGainWDRC_F32.h"  //has definition of CHA_WDRC
//#include "utility/textAndStringUtils.h"

#define db2(x) (AudioCalcGainWDRC_F32::db2(x))  // does 20*log10(x) fast!
#define undb2(x) (AudioCalcGainWDRC_F32::undb2(x)) //does pow(10,x/20) fast!

class AudioEffectExpCompLim_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName: expCompLim
  public:
    AudioEffectExpCompLim_F32(void): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
      setDefaultValues();
    }

    AudioEffectExpCompLim_F32(AudioSettings_F32 settings): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setAudioSettings(settings);
      setDefaultValues();
    }

    void setAudioSettings(AudioSettings_F32 settings) {
      setSampleRate_Hz(settings.sample_rate_Hz);
    }

    //here is the method called automatically by the audio library
    void update(void) {
      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;
      
      //allocate memory for the output of our algorithm
      audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
      if (!out_block) return;
      out_block->length = in_block->length;
      
      //do the algorithm
      do_algorithm(in_block->data, out_block->data, in_block->length);
      
      // transmit the block and release memory
      AudioStream_F32::transmit(out_block); // send the FIR output
      AudioStream_F32::release(out_block);
      AudioStream_F32::release(in_block);
    }


     void do_algorithm(float *x, float *y, int n)    
     //x, input, audio waveform data
     //y, output, audio waveform data after compression
     //n, input, number of samples in this audio block
    {        
        // find smoothed envelope
        audio_block_f32_t *envelope_block = AudioStream_F32::allocate_f32();
        if (!envelope_block) return;
        calcEnvelope.smooth_env(x, envelope_block->data, n);
        envelope_block->length = n;
        
        //calculate gain
        audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
        if (!gain_block) return;
        calcGainFromEnvelope(envelope_block->data, gain_block->data, n);
        gain_block->length = n;

        //apply gain
        arm_mult_f32(x, gain_block->data, y, n);

        // release memory
        AudioStream_F32::release(envelope_block);
        AudioStream_F32::release(gain_block);
    }

    void calcGainFromEnvelope(float *env, float *gain_out, const int n)  {
      //env = input, signal envelope (not the envelope of the power, but the envelope of the signal itslef)
      //gain = output, the gain in natural units (not power, not dB)
      //n = input, number of samples to process in each vector

      //declare some variables to use
      int I_seg = 0;
      float val_dB, gain_dB;
      float32_t diff_rel_thresh_dB=0.f;
  
      //prepare intermediate data block
      audio_block_f32_t *env_dBFS_block = AudioStream_F32::allocate_f32();
      if (!env_dBFS_block) return;
      env_dBFS_block->length = n;
  
      //convert to envelope to dB (relative to full-scale)
      for (int k=0; k < n; k++) env_dBFS_block->data[k] = db2(env[k]); //maxdb in the private section 

      //loop over each sample
      for (int k=0; k < n; k++) {  //loop over each sample
        val_dB = env_dBFS_block->data[k];

        //find the segment of the dynamic processing curve that applies
        I_seg = 2;
        //Serial.print(I_seg); Serial.print(", ");
        //Serial.print((n_segments-1)); Serial.print(", ");
        //Serial.print(val_dB); Serial.print(", ");
        //Serial.print(input_thresh_dBFS[I_seg+1]); Serial.print(", ");
        //Serial.print((I_seg < (n_segments-1)));Serial.print(", ");
        //Serial.print((val_dB < input_thresh_dBFS[I_seg+1]));Serial.print(", ");
        //Serial.println((I_seg < (n_segments-1)) && (val_dB > input_thresh_dBFS[I_seg+1]));
        while ((I_seg < (n_segments-1)) && (val_dB < input_thresh_dBFS[I_seg+1])) { I_seg++; };
        
        // calculate the gain based on this section...assume joint between seg[0] and seg[1] is at linear_gain_dB
        if (I_seg == 0) {
            //assumed to be expansion
            float expand_ratio = 1.0/comp_ratio[I_seg];
            diff_rel_thresh_dB = val_dB - input_thresh_dBFS[1];  //should be negative
            gain_dB = gain_dB_at_transitions[1] + ((expand_ratio-1.f) * diff_rel_thresh_dB);
        } else {
            //can be whatever expand/linear/compression/limit
            diff_rel_thresh_dB = val_dB - input_thresh_dBFS[I_seg];  //should be positive
            gain_dB = gain_dB_at_transitions[I_seg] - (comp_ratio[I_seg]-1.0)*diff_rel_thresh_dB;
        }
      
        //convert from dB into linear gain
        gain_out[k] = undb2(gain_dB);
      }
     
      AudioStream_F32::release(env_dBFS_block);
    }


    void setDefaultValues(void) {
      //overall parameters
      float att_ms = 5.f, rel_ms = 300.f;
      float lin_gain_dB = 0.0;
      setOverallParams(att_ms, rel_ms, lin_gain_dB); 

      //set the parameters for each segment of the compression curve
      setSegmentParams(0,-1000.f, 1./1.75);  //expansion
      setSegmentParams(1,-80.f, 1.);  //linear
      setSegmentParams(2,-60.f, 1.75); //compression
      setSegmentParams(3,-10.f, 5);  //limiter
    }

    void setGain_dB(float gain_dB) {
      linear_gain_dB = gain_dB;
      recomputeGainAtTransitions();
    }

    //set all of the user parameters for the compressor
    //assumes that the sample rate has already been set!!!
    void setOverallParams(float attack_ms, float release_ms, float lin_gain_dB) {
      
      //configure the envelope calculator...assumes that the sample rate has already been set!
      calcEnvelope.setAttackRelease_msec(attack_ms,release_ms);

      //set local private members
      setGain_dB(lin_gain_dB);
    }

    //set parameters for each segment
    void setSegmentParams(int seg_id, float in_thresh_dBFS, float cr) {
      if (seg_id < n_segments) {
        input_thresh_dBFS[seg_id] = in_thresh_dBFS; //threshold for the bottom of this segment
        comp_ratio[seg_id] = cr; //compression ratio (>1.0) or expansion ratio (<1.0) for this segment
      }
      recomputeGainAtTransitions();
    }

    void recomputeGainAtTransitions(void) {
      //assume that the transition between segment[0] and segment[1] is pinned at linear_gain_dB
      gain_dB_at_transitions[0] = 0.0;  //ignored
      gain_dB_at_transitions[1] = linear_gain_dB;  //this is the fixed point for all cases
      for (int i=2; i<n_segments;i++) {
        gain_dB_at_transitions[i] = gain_dB_at_transitions[i-1] - (comp_ratio[i]-1.0)*(input_thresh_dBFS[i]-input_thresh_dBFS[i-1]);
      }

      if (Serial) {
        Serial.print("AudioEffectExpCompLim: recomputeGainAtTransitions: transitions dBFS = "); 
        for (int i=0; i<n_segments;i++) {
          Serial.print(gain_dB_at_transitions[i]); Serial.print(", ");
        }
        Serial.println();
      } 
    }

    void setSampleRate_Hz(const float _fs_Hz) {
      //pass this data on to its components that care
      given_sample_rate_Hz = _fs_Hz;
      calcEnvelope.setSampleRate_Hz(_fs_Hz);
    }

    float getCurrentLevel_dB(void) { return db2(calcEnvelope.getCurrentLevel()); }  //this is 20*log10(abs(signal)) after the envelope smoothing

    AudioCalcEnvelope_F32 calcEnvelope;

    
  private:
    audio_block_f32_t *inputQueueArray[1];
    float given_sample_rate_Hz;
    float linear_gain_dB;
    static const int n_segments = 4;
    float input_thresh_dBFS[n_segments];
    float comp_ratio[n_segments];
    float gain_dB_at_transitions[n_segments];
};


#endif
    

