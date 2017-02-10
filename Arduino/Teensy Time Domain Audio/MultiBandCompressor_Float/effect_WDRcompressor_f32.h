/*
 * effect_WDR_compressor: Wide Dynamic Rnage Compressor
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Derived From: WDRC_circuit from CHAPRO from BTNRC: https://github.com/BTNRH/chapro
 *     As of Feb 2017, CHAPRO license is listed as "Creative Commons?"
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _filter_fir_f32
#define _filter_fir_f32

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"

/* from CHAPRO cha_ff.h
#define DSL_MXCH 32              // maximum number of channels

typedef struct {
    double attack;               // attack time (ms)
    double release;              // release time (ms)
    double maxdB;                // maximum signal (dB SPL)
    int ear;                     // 0=left, 1=right
    int nchannel;                // number of channels
    double cross_freq[DSL_MXCH]; // cross frequencies (Hz)
    double tkgain[DSL_MXCH];     // compression-start gain
    double cr[DSL_MXCH];         // compression ratio
    double tk[DSL_MXCH];         // compression-start kneepoint
    double bolt[DSL_MXCH];       // broadband output limiting threshold
} CHA_DSL;

typedef struct {
    double attack;               // attack time (ms)
    double release;              // release time (ms)
    double fs;                   // sampling rate (Hz)
    double maxdB;                // maximum signal (dB SPL)
    double tkgain;               // compression-start gain
    double tk;                   // compression-start kneepoint
    double cr;                   // compression ratio
    double bolt;                 // broadband output limiting threshold
} CHA_WDRC;
*/

class AudioEffectCompressorWDR_F32 : public AudioStream_F32
{
  public:
    AudioEffectCompressorWDR_F32(void): AudioStream_F32(1,inputQueueArray) {
    }

    //from 
    void cha_agc_channel(CHA_PTR cp, float *x, float *y, int cs) {
      float alfa, beta, *tkgn, *tk, *cr, *bolt, *ppk;
      float *xk, *yk, *pk;
      int k, nc;
  
      // initialize WDRC variables
      alfa = (float) CHA_DVAR[_gcalfa];
      beta = (float) CHA_DVAR[_gcbeta];
      tkgn = (float *) cp[_gctkgn];
      tk = (float *) cp[_gctk];
      cr = (float *) cp[_gccr];
      bolt = (float *) cp[_gcbolt];
      ppk = (float *) cp[_gcppk];
      // loop over channels
      nc = CHA_IVAR[_nc];
      for (k = 0; k < nc; k++) {
          xk = x + k * cs;
          yk = y + k * cs;
          pk = ppk + k;
          compress(cp, xk, yk, cs, pk, alfa, beta, tkgn[k], tk[k], cr[k], bolt[k]);
      }
    }

    
    void setAttackRelease_sec(float atk_sec, float rel_sec, float fs_Hz) {
      time_const(atk_sec, rel_sec, fs_Hz, &alfa, &beta);
    }


  private:
    float alfa, beta;
    
    //from CHAPRO, agc_prepare.c
    static void time_const(double atk_mec, double rel_mec, double fs, float *alfa, float *beta) {
        double ansi_atk, ansi_rel;
    
        // convert ANSI attack & release times to filter time constants
        ansi_atk = atk * fs / 2.425f; 
        ansi_rel = rel * fs / 1.782f; 
        *alfa = (float) (ansi_atk / (1.0f + ansi_atk));
        *beta = (float) (ansi_rel / (10.f + ansi_rel));
    }
};
    
