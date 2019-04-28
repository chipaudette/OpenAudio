/*
*   TrebleBoost_wComp
*
*   Created: Chip Audette, OpenAudio, Apr 2017
*   Purpose: Process audio by applying a high-pass filter followed by gain followed
*            by a dynamic range compressor.
*
*   Uses Tympan Audio Adapter.
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*   
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Audio.h>           //include the Teensy Audio Library
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 4;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
AudioControlAIC3206       audioHardware;
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC. 
AudioFilterBiquad_F32     hp_filt1(audio_settings);
AudioFilterBiquad_F32     lp_filt1(audio_settings);   //IIR filter doing a lowpass filter.  Left.
//AudioFilterBiquad_F32     lp_filt2(audio_settings);   //IIR filter doing a lowpass filter.  Right.
AudioEffectGain_F32       gain1(audio_settings);
//AudioEffectGain_F32       gain2(audio_settings);
AudioEffectCompWDRC_F32   comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.
//AudioEffectCompWDRC_F32   comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC. 

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input to the left highpass filter
AudioConnection_F32       patchCord10(hp_filt1, 0, lp_filt1, 0);
//AudioConnection_F32       patchCord2(i2s_in, 1, lp_filt2, 0);   //connect the Right input to the right highpass filter
AudioConnection_F32       patchCord3(lp_filt1, 0, gain1, 0);    //connect to the gain to invert
//AudioConnection_F32       patchCord4(lp_filt2, 0, gain2, 0);    //connect to the gain to invert
AudioConnection_F32       patchCord5(gain1, 0, comp1, 0);       //connect to the compressor/limiter
//AudioConnection_F32       patchCord6(gain2, 0, comp2, 0);       //connect to the compressor/limiter
AudioConnection_F32       patchCord7(comp1, 0, i2s_out, 0);     //connect to the Left output
AudioConnection_F32       patchCord8(comp1, 0, i2s_out, 1);     //connect to the Right output

//AudioConnection_F32       patchCord8(comp2, 0, i2s_out, 1);     //connect to the Right output

//define a function to configure the left and right compressors
void setupMyCompressors(void) {
  //set the speed of the compressor's response
  float attack_msec = 5.0;
  float release_msec = 300.0;
  comp1.setAttackRelease_msec(attack_msec, release_msec); //left channel
  //comp2.setAttackRelease_msec(attack_msec, release_msec); //right channel

  //Single point system calibration.  what is the SPL of a full scale input (including effect of input_gain_dB)?
  float SPL_of_full_scale_input = 115.0;  
  comp1.setMaxdB(SPL_of_full_scale_input);  //left channel
  //comp2.setMaxdB(SPL_of_full_scale_input);  //right channel

  //now define the compression parameters
  float knee_compressor_dBSPL = 85.0;  //follows setMaxdB() above
  float comp_ratio = 1.0;
  comp1.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //left channel
  //comp2.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //right channel
  comp1.setCompRatio(comp_ratio); //left channel
  //comp2.setCompRatio(comp_ratio);

  gain1.setGain_dB(10.0);

  //finally, the WDRC module includes a limiter at the end.  set its parameters
  float knee_limiter_dBSPL = SPL_of_full_scale_input - 20.0;  //follows setMaxdB() above
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp1.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //left channel
  //comp2.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //right channel
  
}

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("ANC via feedback with LP and Comp: Starting setup()...");
  
  //allocate the audio memory first
  AudioMemory(10); AudioMemory_F32(10,audio_settings); //allocate both kinds of memory

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC
  
  //Choose the desired input
  //audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the filters
  float hp_cutoff_Hz = 40.f;
  float lp_cutoff_Hz = 250.f;
  Serial.print("Higpass filter cutoff at ");Serial.print(hp_cutoff_Hz);Serial.println(" Hz");
  Serial.print("Lowpass filter cutoff at ");Serial.print(lp_cutoff_Hz);Serial.println(" Hz");
  hp_filt1.setLowpass(0, hp_cutoff_Hz); //biquad IIR filter.  left channel
  lp_filt1.setLowpass(0, lp_cutoff_Hz); //biquad IIR filter.  left channel
  //lp_filt2.setLowpass(0, cutoff_Hz); //biquad IIR filter.  right channel

  //invert the gain
  gain1.setGain(5.0f);
  //gain1.setGain(-1.0);
  //gain2.setGain(-1.0);

  //configure the left and right compressors with the desired settings
  setupMyCompressors();

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

  // check the setting on the potentiometer
  servicePotentiometer(millis(),0);
  
  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //periodically check the potentiometer
  servicePotentiometer(millis(),100); //update every 100 msec

  //check to see whether to print the CPU and Memory Usage
  printCPUandMemory(millis(),6000); //print every 3000 msec 
  
  //periodically print the gain status
  printGainStatus(millis(),2000); //update every 4000 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = (1.0/25.0) * (float)((int)(25.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around
      val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)

      switch (3) {
        case (1):
          {
          //choose the desired gain value based on the knob setting
          const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
          vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB
          
          //command the new gain setting
          float vol_knob_gain = powf(10.0,vol_knob_gain_dB/20.f);
          gain1.setGain(-vol_knob_gain);  //set gain (and invert)
          //gain2.setGain(-vol_knob_gain);  //set gain (and invert)
          Serial.print("servicePotentiometer: inverted gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
          }
          break;
        case (2):
          {
          const float min_val = 100.0, max_val = 500.0; //set desired range
          float freq_Hz = min_val + (max_val - min_val)*val; //computed desired gain value in dB
          
          //command the new gain setting
          lp_filt1.setLowpass(0,freq_Hz);  //set gain (and invert)
          Serial.print("servicePotentiometer: lowpass Hz= "); Serial.println(freq_Hz); //print text to Serial port for debugging
          }
          break;
        case (3):
          {
          float gain =5.0;
          if (val > 0.5) {
            gain = -gain;
          }
          gain1.setGain(gain);
          Serial.print("servicePotentiometer: setting gain to "); Serial.println(gain);       
          }
          break;
      }
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("printCPUandMemory: ");
    Serial.print("CPU Cur/Peak: ");
    Serial.print(audio_settings.processorUsage());
    //Serial.print(AudioProcessorUsage()); //if not using AudioSettings_F32
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
    //Serial.print(AudioProcessorUsageMax());  //if not using AudioSettings_F32
    Serial.print("%,   ");
    Serial.print("Dyn MEM Int16 Cur/Peak: ");
    Serial.print(AudioMemoryUsage());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax());
    Serial.print(",   ");
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

//This routine plots the current gain settings, including the dynamically changing gains
//of the compressors
void printGainStatus(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("printGainStatus: ");
    
    Serial.print("Input PGA = ");
    Serial.print(input_gain_dB,1);
    Serial.print(" dB.");
    
    Serial.print(" Compressor Gain (L/R) = "); 
    Serial.print(comp1.getCurrentGain_dB(),1);
    //Serial.print(", ");
    //Serial.print(comp2.getCurrentGain_dB(),1);
    Serial.print(" dB.");
  
    Serial.println();
   
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
