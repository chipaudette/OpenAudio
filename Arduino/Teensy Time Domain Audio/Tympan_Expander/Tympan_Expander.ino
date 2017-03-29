#include <Tympan_Library.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "AudioEffectExpander.h"
//include "AudioMultiblockAverage.h"

const float sample_rate_Hz = 24000.f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 32;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

const float input_gain_dB = 20.0f; //gain on the microphone


// GUItool: begin automatically generated code
AudioInputI2S_F32        audioInI2S1(audio_settings);    //xy=126,110
AudioFilterFIR_F32       fir[3];           //xy=223,216
AudioEffectExpander_F32     expander3(audio_settings);         //xy=394,308
AudioEffectExpander_F32     expander1(audio_settings);         //xy=395,217
AudioEffectExpander_F32     expander2(audio_settings);         //xy=398,261
AudioMixer4_F32          mixer4_1;       //xy=587,275
AudioEffectCompressor_F32  limiter1;     //xy=717,184
AudioOutputI2S_F32       audioOutI2S1(audio_settings);   //xy=860,104
AudioControlTLV320AIC3206 audioHardware; //xy=161,42
AudioConfigFIRFilterBank_F32 configFIRFilterBank1; //xy=349,45

AudioConnection_F32         patchCord1(audioInI2S1, 0, fir[0], 0);
AudioConnection_F32         patchCord2(audioInI2S1, 0, fir[1], 0);
AudioConnection_F32         patchCord3(audioInI2S1, 0, fir[2], 0);
AudioConnection_F32         patchCord11(fir[0], 0, expander1, 0);
AudioConnection_F32         patchCord12(fir[1], 0, expander2, 0);
AudioConnection_F32         patchCord13(fir[2], 0, expander3, 0);
AudioConnection_F32         patchCord21(expander1, 0, mixer4_1, 0);
AudioConnection_F32         patchCord22(expander2, 0, mixer4_1, 1);
AudioConnection_F32         patchCord23(expander3, 0, mixer4_1, 2);
AudioConnection_F32         patchCord10(mixer4_1, limiter1);
AudioConnection_F32         patchCord31(limiter1, 0, audioOutI2S1, 0);
AudioConnection_F32         patchCord32(limiter1, 0, audioOutI2S1, 1);


// GUItool: end automatically generated code

#define POT_PIN A1

//setup the tympan
void setupAudioHardware(void) {
    // Setup the TLV320
  audioHardware.enable(); // activate AIC

  // Choose the desired input
  //tlv320aic3206_1.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones // default
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  //  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line in pads on the TYMPAN board - defaults to mic bias OFF

  //Adjust the MIC bias, if using TYMPAN_INPUT_JACK_AS_MIC
  //  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_OFF); // Turn mic bias off
  audioHardware.setMicBias(TYMPAN_MIC_BIAS_2_5); // set mic bias to 2.5 // default
  //  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_1_7); // set mic bias to 1.7
  //  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_1_25); // set mic bias to 1.25
  //  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_VSUPPLY); // set mic bias to supply voltage

  // set initial VOLUMES
  audioHardware.volume_dB(0);  // -63.6 to +24 dB in 0.5dB steps.  uses float
  audioHardware.setInputGain_dB(input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps

  //setup the potentiometer as an input
  pinMode(POT_PIN,INPUT);
}

#define N_CHAN 3
#define N_FIR 96
#define FS_HZ audio_settings.sample_rate_Hz
float fir_coeff[N_CHAN][N_FIR];

//setup algorithm parameters
void setupAlgorithms(void) {
  //setup the filters
  float corner_freq[] = { 500.f, 1500.f};
  configFIRFilterBank1.createFilterCoeff(N_CHAN, N_FIR, FS_HZ, &corner_freq[0], &fir_coeff[0][0]);
//  for (int i=0; i< N_CHAN;i++) { 
//    for (int j=0; j<N_FIR; j++) { 
//      fir_coeff[i][j] = 0.0; 
//    }
//  }
//  fir_coeff[0][0] = 1.0;
  for (int i=0; i< N_CHAN; i++) fir[i].begin(fir_coeff[i], N_FIR, audio_settings.audio_block_samples);

  //setup the expanders
  float attack_ms = 5.f, release_ms = 300.f;
  float thresh_dBFS[] = {-50.0f, -83.0f+2.0f, -73.f+2.0f};
  float expand_ratio[] = {1.f, 1.75f, 1.75f};
  float linear_gain_dB = 0.f; //applied after comparison to threshold
  expander1.setParams(attack_ms, release_ms, thresh_dBFS[0], expand_ratio[0], linear_gain_dB);
  expander2.setParams(attack_ms, release_ms, thresh_dBFS[1], expand_ratio[1], linear_gain_dB);
  expander3.setParams(attack_ms, release_ms, thresh_dBFS[2], expand_ratio[2], linear_gain_dB);

  //setup the final compressor as a limitter
  limiter1.setAttack_sec(attack_ms / 1000.f, FS_HZ);
  limiter1.setRelease_sec(release_ms / 1000.f, FS_HZ);
  limiter1.setPreGain_dB(0.f);
  limiter1.setThresh_dBFS(-10.f);
  limiter1.setCompressionRatio(1.f);
      
}

//The setup function is called once when the system starts up
void setup(void) {
  //Start the USB serial link (to enable debugging)
  Serial.begin(115200); delay(500);
  Serial.println("Setup starting...");
  
  //Allocate dynamically shuffled memory for the audio subsystem
  AudioMemory(10);  AudioMemory_F32_wSettings(10,audio_settings);
  
  //Put your own setup code here
  setupAudioHardware();
  setupAlgorithms();
  
  //End of setup
  Serial.println("Setup complete.");
};


//After setup(), the loop function loops forever.
//Note that the audio modules are called in the background.
//They do not need to be serviced by the loop() function.
void loop(void) {
   //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis());

  //update the memory and CPU usage...if enough time has passed
  printMemoryAndCPU(millis());

  //print info about the signal processing
  printUpdatedLevels(millis());
}



//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = 1.0/9.0 * (float)((int)(9.0* val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0
    
    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around
      val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)
      
      float total_gain_dB = val*45.0;  //span 0 to 45
      float linear_gain_dB = total_gain_dB - input_gain_dB;
      Serial.print("Total Gain = "); Serial.print(total_gain_dB); Serial.print(" dB, ");
      Serial.print("Mic Gain = "); Serial.print(input_gain_dB); Serial.print(" dB, ");
      Serial.print("Exp Linear Gain = "); Serial.print(linear_gain_dB); Serial.println();
      expander1.setGain_dB(linear_gain_dB); expander2.setGain_dB(linear_gain_dB);expander3.setGain_dB(linear_gain_dB);

    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();

void printMemoryAndCPU(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("CPU Cur/Peak: ");
    //Serial.print(audio_settings.processorUsage());
    Serial.print(AudioProcessorUsage());
    Serial.print("%/");
    //Serial.print(audio_settings.processorUsageMax());
    Serial.print(AudioProcessorUsageMax());
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

void printUpdatedLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 1000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;
  static float ave_dB[3];
  float update_coeff = 0.2;
  

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    ave_dB[0] = (1.0-update_coeff)*ave_dB[0] + update_coeff*expander1.getCurrentLevel_dB();
    ave_dB[1] = (1.0-update_coeff)*ave_dB[1] + update_coeff*expander2.getCurrentLevel_dB();
    ave_dB[2] = (1.0-update_coeff)*ave_dB[2] + update_coeff*expander3.getCurrentLevel_dB();

    Serial.print("Signal Level Per-Band (dBFS) = ");
    for (int i=0; i<3; i++) { Serial.print(ave_dB[i]);  Serial.print(", ");  }
    Serial.println();
    
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

