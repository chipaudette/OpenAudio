/*
  Tympan_Ultrasound

  Created: Chip Audette (OpenAudio), Feb 2017
  
  Purpose: Sample fast enough (96 kHz) to acquire ultrasound signals (up to 48 kHz)
      then do single side-band shifting of the signals down to the audible range

  Uses Teensy Audio Adapter ro the Tympan Audio Board
      For Teensy Audio Board, assumes microphones (or whatever) are attached to the
      For Tympan Audio Board, can use on board mics or external mic

  User Controls:
    Potentiometer on Tympan controls the amount of frequency shift

   MIT License.  use at your own risk.
*/

//Use the new Tympan board?
#define USE_TYMPAN 1    //1 = uses tympan hardware, 0 = uses the old teensy audio board

//Use test tone as input (set to 1)?  Or, use live audio (set to zero)
#define USE_TEST_TONE_INPUT 0

//do stereo processing?
#define USE_STEREO 0

// Include all the of the needed libraries
//include <Audio.h>      //Teensy Audio Library
#include <Wire.h>
#include <SPI.h>
//include <SD.h>
//include <SerialFlash.h>
#include <Tympan_Library.h> //for AudioConvert_I16toF32, AudioConvert_F32toI16, and AudioEffectGain_F32


const float sample_rate_Hz = 96000.0f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 128;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
#if USE_TYMPAN == 0
  AudioControlSGTL5000        audioHardware;    //controller for the Teensy Audio Board
#else
  AudioControlAIC3206   audioHardware;    //controller for the Teensy Audio Board
#endif
AudioSynthWaveformSine_F32  testSignal(audio_settings);          //use to generate test tone as input
AudioInputI2S_F32           i2s_in(audio_settings);          //Digital audio *from* the Teensy Audio Board ADC.  Sends Int16.  Stereo.

// GUItool: begin automatically generated code
AudioEffectGain_F32      preGainL, preGainR;          //xy=176,192
AudioFilterBiquad_F32       iirL1, iirL2;           //xy=271,258
AudioFilterBiquad_F32       iirR1, iirR2;           //xy=280,275
AudioSynthWaveformSine_F32 sine2(audio_settings);          //xy=275,397
AudioMathMultiply_F32        multiplyL, multiplyR;      //xy=408,318
AudioMixer4_F32             mixerL, mixerR;
AudioEffectCompWDRC_F32  compWDRC1;      //xy=427,172
AudioOutputI2S_F32          i2s_out(audio_settings);        //Digital audio *to* the Teensy Audio Board DAC.  Expects Int16.  Stereo

#if USE_TEST_TONE_INPUT
  AudioConnection_F32         patchCord1(testSignal, 0, preGainL, 0);
  #if USE_STEREO
    AudioConnection_F32         patchCord101(testSignal, 0, preGainR, 0);
  #endif
#else
  AudioConnection_F32         patchCord1(i2s_in, 0, preGainL, 0); 
  #if USE_STEREO
    AudioConnection_F32         patchCord101(i2s_in, 1, preGainR, 0);
  #endif
#endif
AudioConnection_F32         patchCord2(preGainL, 0, iirL1, 0);
AudioConnection_F32         patchCord3(iirL1, 0, iirL2, 0);
AudioConnection_F32         patchCord10(iirL2, 0, multiplyL, 0);
AudioConnection_F32         patchCord4(sine2, 0, multiplyL, 1);
AudioConnection_F32         patchCord20(i2s_in, 0, mixerL, 0);
AudioConnection_F32         patchCord21(multiplyL, 0, mixerL, 1);
AudioConnection_F32         patchCord27(mixerL, 0, compWDRC1, 0);
AudioConnection_F32         patchCord5(compWDRC1, 0, i2s_out, 0);

#if USE_STEREO
  AudioConnection_F32         patchCord200(preGainR, 0, iirR1, 0);
  AudioConnection_F32         patchCord300(iirR1, 0, iirR2, 0);
  AudioConnection_F32         patchCord1000(iirR2, 0, multiplyR, 0);
  AudioConnection_F32         patchCord400(sine2, 0, multiplyR, 1);
  AudioConnection_F32         patchCord2000(i2s_in, 1, mixerR, 0);
  AudioConnection_F32         patchCord2100(multiplyR, 0, mixerR, 1);
  AudioConnection_F32         patchCord500(mixerR, 0, i2s_out, 1);
#else
  AudioConnection_F32         patchCord6(compWDRC1, 0, i2s_out, 1);
#endif


// GUItool: end automatically generated code

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define functions to setup the hardware
void setupAudioHardware(void) {
  #if USE_TYMPAN == 0
    //use Teensy Audio Board
    Serial.println("Setting up Teensy Audio Board...");
    const int myInput = AUDIO_INPUT_LINEIN;   //which input to use?  AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
    audioHardware.enable();                   //start the audio board
    audioHardware.inputSelect(myInput);       //choose line-in or mic-in
    audioHardware.volume(0.8);                //volume can be 0.0 to 1.0.  0.5 seems to be the usual default.
    audioHardware.lineInLevel(10, 10);        //level can be 0 to 15.  5 is the Teensy Audio Library's default
    audioHardware.adcHighPassFilterDisable(); //reduces noise.  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
  #else
    //use Tympan Audio Board
    Serial.println("Setting up Tympan Audio Board...");
    audioHardware.enable(); // activate AIC
  
    //choose input
    //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack
    audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones // default
  
    //choose mic bias (if using mics on input jack)
    int myBiasLevel = TYMPAN_MIC_BIAS_2_5;  //choices: TYMPAN_MIC_BIAS_2_5, TYMPAN_MIC_BIAS_1_7, TYMPAN_MIC_BIAS_1_25, TYMPAN_MIC_BIAS_VSUPPLY
    audioHardware.setMicBias(myBiasLevel); // set mic bias to 2.5 // default
  
    //set volumes
    audioHardware.volume_dB(0);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
    audioHardware.setInputGain_dB(20); // set MICPGA volume, 0-47.5dB in 0.5dB setps
  #endif

  //setup the potentiometer.  same for Teensy Audio Board as for Tympan
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT
}

//define my high-pass filter: [b,a]=butter(2,30000/(96000/2),'high')
float32_t hp_b[] = {0.186694333116378,  -0.373388666232757,   0.186694333116378};
float32_t hp_a[] = { 1.000000000000000,   0.462938025291041,   0.209715357756555};

float32_t carrier_freq_Hz = 37000.0f;

//define functions to setup the audio processing parameters
void setupAudioProcessing(void) {
  //set the pre-gain (if used)
  preGainL.setGain_dB(10.0f);
  preGainR.setGain_dB(10.0f);

  //setup the HP filter
  iirL1.setFilterCoeff_Matlab(hp_a, hp_b);
  iirL2.setFilterCoeff_Matlab(hp_a, hp_b); //appply the same filter a second time for steeper roll-off
  iirR1.setFilterCoeff_Matlab(hp_a, hp_b);
  iirR2.setFilterCoeff_Matlab(hp_a, hp_b); //appply the same filter a second time for steeper roll-off

  //setup the demodulation
  sine2.amplitude(1.0);
  sine2.frequency(carrier_freq_Hz);

  //setup mixer
  mixerL.gain(0,0.0); //normal audio is channel 0
  mixerL.gain(1,1.0); //demodulated ultrasound is channel 1
  mixerR.gain(0,0.0); //normal audio is channel 0
  mixerR.gain(1,1.0); //demodulated ultrasound is channel 1

}

// define the setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //Open USB Serial link...for debugging
  delay(500); 
  
  Serial.println("Ultrasound: setup()...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory(40);      //allocate Int16 audio data blocks (need a few for under-the-hood stuff)
  AudioMemory_F32_wSettings(40,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupAudioHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //setup sine wave as test signal..if the sine input
  testSignal.amplitude(0.01);
  testSignal.frequency(500.0f);
  Serial.println("setup() complete");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis());

  //update the memory and CPU usage...if enough time has passed
  printMemoryAndCPU(millis());

} //end loop()


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
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around
      if (USE_TYMPAN == 1) val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)

      #if USE_TEST_TONE_INPUT==1
            float freq = carrier_freq_Hz *(1.0f + 0.125f * ((val-0.5) * 2.0)); //change tone carrier_Hz +/- 25%
            Serial.print("Changing tone frequency to = "); Serial.println(freq);
            testSignal.frequency(freq);
      #else
        #if USE_TYMPAN == 1
          #if 0
            //change the volume
            float vol_dB = 0.f + 30.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 30 dB
            Serial.print("Changing output volume frequency to = "); Serial.print(vol_dB); Serial.println(" dB");
            audioHardware.volume_dB(vol_dB);
          #else
            //change the carrier
            float freq = 30000 + 10000.f*val; //change tone carrier_Hz 30000-40000
            Serial.print("Changing carruer frequency to = "); Serial.println(freq);
            sine2.frequency(freq);
            if (val < 0.025) {
              mixerL.gain(0,1.0);  mixerL.gain(1,0.0); //switch to normal audio
              mixerR.gain(0,1.0);  mixerR.gain(1,0.0); //switch to normal audio
            } else {
              mixerL.gain(0,0.0);  mixerL.gain(1,1.0); //switch to demodulated ultrasound
              mixerR.gain(0,0.0);  mixerR.gain(1,1.0); //switch to demodulated ultrasound
            }
          #endif
        #else
              float vol = 0.70f + 0.15f * ((val - 0.5) * 2.0); //set volume as 0.70 +/- 0.15
              Serial.print("Setting output volume control to = "); Serial.println(vol);
              audioHardware.volume(vol);
        #endif
      #endif
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
    Serial.print(audio_settings.processorUsage());
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
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

