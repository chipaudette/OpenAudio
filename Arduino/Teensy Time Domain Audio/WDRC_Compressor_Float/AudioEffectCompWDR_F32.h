/*
 * AudioEffectCompWDR_F32: Wide Dynamic Rnage Compressor
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Derived From: WDRC_circuit from CHAPRO from BTNRC: https://github.com/BTNRH/chapro
 *     As of Feb 2017, CHAPRO license is listed as "Creative Commons?"
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _AudioEffectCompWDR_F32
#define _AudioEffectCompWDR_F32

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"


// from CHAPRO cha_ff.h
#define DSL_MXCH 32              // maximum number of channels
typedef struct {
    float attack;               // attack time (ms)
    float release;              // release time (ms)
    float maxdB;                // maximum signal (dB SPL)
    int ear;                     // 0=left, 1=right
    int nchannel;                // number of channels
    float cross_freq[DSL_MXCH]; // cross frequencies (Hz)
    float tkgain[DSL_MXCH];     // compression-start gain
    float cr[DSL_MXCH];         // compression ratio
    float tk[DSL_MXCH];         // compression-start kneepoint
    float bolt[DSL_MXCH];       // broadband output limiting threshold
} CHA_DSL;

typedef struct {
    float attack;               // attack time (ms)
    float release;              // release time (ms)
    float fs;                   // sampling rate (Hz)
    float maxdB;                // maximum signal (dB SPL)
    float tkgain;               // compression-start gain
    float tk;                   // compression-start kneepoint
    float cr;                   // compression ratio
    float bolt;                 // broadband output limiting threshold
} CHA_WDRC;

typedef struct {
    float alfa;                 // attack constant (not time)
    float beta;                 // release constant (not time
    float fs;                   // sampling rate (Hz)
    float maxdB;                // maximum signal (dB SPL)
    float tkgain;               // compression-start gain
    float tk;                   // compression-start kneepoint
    float cr;                   // compression ratio
    float bolt;                 // broadband output limiting threshold
} CHA_DVAR_t;


#define undb2(x)    (expf(0.11512925464970228420089957273422f*x))  //faster:  exp(log(10.0f)*x/20);  this is exact
#define db2(x)      (6.020599913279623f*log2f_approx(x)) //faster: 20*log2_approx(x)/log2(10);  this is approximate
/* ----------------------------------------------------------------------
** Fast approximation to the log2() function.  It uses a two step
** process.  First, it decomposes the floating-point number into
** a fractional component F and an exponent E.  The fraction component
** is used in a polynomial approximation and then the exponent added
** to the result.  A 3rd order polynomial is used and the result
** when computing db20() is accurate to 7.984884e-003 dB.
** ------------------------------------------------------------------- */
//https://community.arm.com/tools/f/discussions/4292/cmsis-dsp-new-functionality-proposal/22621#22621
static float log2f_approx(float X) {
  //float *C = &log2f_approx_coeff[0];
  float Y;
  float F;
  int E;

  // This is the approximation to log2()
  F = frexpf(fabsf(X), &E);
  //  Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
  Y = 1.23149591368684f; //C[0]
  Y *= F;
  Y += -4.11852516267426f;  //C[1]
  Y *= F;
  Y += 6.02197014179219f;  //C[2]
  Y *= F;
  Y += -3.13396450166353f; //C[3]
  Y += E;

  return(Y);
}


class AudioEffectCompWDR_F32 : public AudioStream_F32
{
  public:
    AudioEffectCompWDR_F32(void): AudioStream_F32(1,inputQueueArray) {
      //set default values...taken from CHAPRO, GHA_Demo.c  from "amplify()"
      CHA_WDRC gha = {1.0f, // attack time (ms)
        50.0f,     // release time (ms)
        24000.0f,  // fs, sampling rate (Hz)
        119.0f,    // maxdB, maximum signal (dB SPL)
        0.0f,      // tkgain, compression-start gain
        105.0f,    // tk, compression-start kneepoint
        10.0f,     // cr, compression ratio
        105.0f     // bolt, broadband output limiting threshold
      };
      setParams(gha.attack, gha.release, gha.fs, gha.maxdB, gha.tkgain, gha.cr, gha.tk, gha.bolt);
    }

    //here is the method called automatically by the audio library
    void update(void) {
      //receive the input audio data
      audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
      if (!block) return;
      
      //allocate memory for the output of our algorithm
      audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
      if (!out_block) return;
      
      //do the algorithm
      cha_agc_channel(block->data, out_block->data, block->length);
      
      // transmit the block and release memory
      AudioStream_F32::transmit(out_block); // send the FIR output
      AudioStream_F32::release(out_block);
      AudioStream_F32::release(block);
    }


    //here is the function that does all the work
    void cha_agc_channel(float *input, float *output, int cs) {  
      //compress(input, output, cs, &prev_env,
      //  CHA_DVAR.alfa, CHA_DVAR.beta, CHA_DVAR.tkgain, CHA_DVAR.tk, CHA_DVAR.cr, CHA_DVAR.bolt, CHA_DVAR.maxdB);
      compress(input, output, cs);
    }

    //void compress(float *x, float *y, int n, float *prev_env,
    //    float &alfa, float &beta, float &tkgn, float &tk, float &cr, float &bolt, float &mxdB)
     void compress(float *x, float *y, int n)    
    {
        // find smoothed envelope
        const float alfa = CHA_DVAR.alfa;
        const float beta = CHA_DVAR.beta;
        //xpk = (float *) cp[_xpk];
        audio_block_f32_t *envelope = AudioStream_F32::allocate_f32();
        float *xpk = envelope->data; //get pointer to the array of (empty) data values
        //smooth_env(x, xpk, n, ppk, alfa, beta);
        smooth_env(x, xpk, n, &prev_env, alfa, beta);
        
        // convert envelope to dB
        //mxdb = (float) CHA_DVAR[_mxdb];
        const float mxdb = CHA_DVAR.maxdB;
        for (int k = 0; k < n; k++) xpk[k] = mxdb + db2(xpk[k]);
        
        // apply wide-dynamic range compression
        const float tkgn = CHA_DVAR.tkgain;
        const float tk = CHA_DVAR.tk;
        const float cr = CHA_DVAR.cr;
        const float bolt = CHA_DVAR.bolt;
        WDRC_circuit(x, y, xpk, n, tkgn, tk, cr, bolt);

        // release memory
        AudioStream_F32::release(envelope);
    }

    void smooth_env(float *x, float *y, int n, float *ppk, float alfa, float beta)
    {
        float  xab, xpk;
        int k;
    
        // find envelope of x and return as y
        xpk = *ppk;                     // start with previous xpk
        for (k = 0; k < n; k++) {
          xab = (x[k] >= 0) ? x[k] : -x[k];
          if (xab >= xpk) {
              xpk = alfa * xpk + (1-alfa) * xab;
          } else {
              xpk = beta * xpk;
          }
          y[k] = xpk;
        }
        *ppk = xpk;                     // save xpk for next time
    }

    void WDRC_circuit(float *x, float *y, float *pdb, int n, float tkgn, float tk, float cr, float bolt)
    {
      float gdb, tkgo, pblt;
      int k;
      
      if ((tk + tkgn) > bolt) {
          tk = bolt - tkgn;
      }
      tkgo = tkgn + tk * (1 - 1 / cr);
      pblt = cr * (bolt - tkgo);
      for (k = 0; k < n; k++) {
        if ((pdb[k] < tk) && (cr >= 1)) {
            gdb = tkgn;
        } else if (pdb[k] > pblt) {
            gdb = bolt + ((pdb[k] - pblt) / 10) - pdb[k];
        } else {
            gdb = ((1 / cr) - 1) * pdb[k] + tkgo;
        }
        y[k] = x[k] * undb2(gdb); 
      }
    }

    //set all of the user parameters for the compressor
    void setParams(float attack_ms, float release_ms, float fs_Hz, float maxdB, float tkgain, float comp_ratio, float tk, float bolt) {
      time_const(attack_ms, release_ms, fs_Hz, &CHA_DVAR.alfa, &CHA_DVAR.beta);
      CHA_DVAR.fs = fs_Hz;
      CHA_DVAR.maxdB = maxdB;
      CHA_DVAR.tkgain = tkgain;
      CHA_DVAR.cr = comp_ratio;
      CHA_DVAR.tk = tk;
      CHA_DVAR.bolt = bolt;
    }

    //convert time constants from seconds to unitless parameters
    //from CHAPRO, agc_prepare.c
    static void time_const(float atk_msec, float rel_msec, float fs, float *alfa, float *beta) {
        float ansi_atk, ansi_rel;
    
        // convert ANSI attack & release times to filter time constants
        ansi_atk = 0.001f* atk_msec * fs / 2.425f; 
        ansi_rel = 0.001f* rel_msec * fs / 1.782f; 
        *alfa = (float) (ansi_atk / (1.0f + ansi_atk));
        *beta = (float) (ansi_rel / (10.f + ansi_rel));
    }

  private:
    audio_block_f32_t *inputQueueArray[1];
    CHA_DVAR_t CHA_DVAR;
    float32_t prev_env = 0.f;  // "ppk" in CHAPRO
};


static void configureBroadbandWDRCs(int ncompressors, float fs_Hz, CHA_WDRC *gha, AudioEffectCompWDR_F32 *WDRCs) {
  //assume all broadband compressors are the same
  for (int i=0; i< ncompressors; i++) {
    //logic and values are extracted from from CHAPRO repo agc_prepare.c...the part setting CHA_DVAR
    
    //extract the parameters
    float atk = (float)gha->attack;  //milliseconds!
    float rel = (float)gha->release; //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override...not taken from gha
    float maxdB = (float) gha->maxdB;
    float tk = (float) gha->tk;
    float comp_ratio = (float) gha->cr;
    float tkgain = (float) gha->tkgain;
    float bolt = (float) gha->bolt;
    
    //set the compressor's parameters
    WDRCs[i].setParams(atk,rel,fs,maxdB,tkgain,comp_ratio,tk,bolt);    
  }
}
    
static void configurePerBandWDRCs(int nchan, float fs_Hz, CHA_DSL *dsl, CHA_WDRC *gha, AudioEffectCompWDR_F32 *WDRCs) {
  if (nchan > dsl->nchannel) {
    Serial.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    Serial.print(F("    : nchan = ")); Serial.println(nchan);
    Serial.print(F("    : dsl.nchannel = ")); Serial.println(dsl->nchannel);
  }
  
  //now, loop over each channel
  for (int i=0; i < nchan; i++) {
    
    //logic and values are extracted from from CHAPRO repo agc_prepare.c
    float atk = (float)dsl->attack;   //milliseconds!
    float rel = (float)dsl->release;  //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override
    float maxdB = (float) gha->maxdB;
    float tk = (float) dsl->tk[i];
    float comp_ratio = (float) dsl->cr[i];
    float tkgain = (float) dsl->tkgain[i];
    float bolt = (float) dsl->bolt[i];

    // adjust BOLT
    float cltk = (float)gha->tk;
    if (bolt > cltk) bolt = cltk;
    if (tkgain < 0) bolt = bolt + tkgain;

    //set the compressor's parameters
    WDRCs[i].setParams(atk,rel,fs,maxdB,tkgain,comp_ratio,tk,bolt);
  }  
}

static void configureMultiBandWDRCasGHA(float fs_Hz, CHA_DSL *dsl, CHA_WDRC *gha, 
    int nBB, AudioEffectCompWDR_F32 *broadbandWDRCs,
    int nchan, AudioEffectCompWDR_F32 *perBandWDRCs) {
    
  configureBroadbandWDRCs(nBB, fs_Hz, gha, broadbandWDRCs);
  configurePerBandWDRCs(nchan, fs_Hz, dsl, gha, perBandWDRCs);
}

#endif
    
