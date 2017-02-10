/*
   MultiBandCompressor_Float

   Created: Chip Audette (openAudio), Feb 2017
   Purpose: Implements 8-band compressor, all in the time-domain

   Uses Teensy Audio Adapter ro the Tympan Audio Board
      For Teensy Audio Board, assumes microphones (or whatever) are attached to the
      For Tympan Audio Board, can use on board mics or external mic

   Listens  potentiometer mounted to Audio Board to provde a control signal.

   MIT License.  use at your own risk.
*/

#define CUSTOM_SAMPLE_RATE 24000     //See local AudioStream_Mod.h.  Only a limitted number supported
#define CUSTOM_BLOCK_SAMPLES 128     //See local AudioStream_Mod.h.  Do not change.  Doesn't work yet.

//define some processing options
#define USE_PER_CHANNEL_AGC 1

//Use the new Tympan board?
#define USE_TYMPAN 1    //1 = uses tympan hardware, 0 = uses teensy audio board

//Use test tone as input (set to 1)?  Or, use live audio (set to zero)
#define USE_TEST_TONE_INPUT 0

//include my custom AudioStream.h...this prevents the default one from being used
#include "AudioStream_Mod.h"

//These are the includes from the Teensy Audio Library
#include <Audio.h>      //Teensy Audio Library
#include <Wire.h>
#include <SPI.h>
//include <SD.h>
//include <SerialFlash.h>

#include <OpenAudio_ArduinoLibrary.h> //for AudioConvert_I16toF32, AudioConvert_F32toI16, and AudioEffectGain_F32
#include "filter_fir_f32.h"


//create audio library objects for handling the audio
#if USE_TYMPAN == 0
AudioControlSGTL5000    audioHardware;    //controller for the Teensy Audio Board
#else
#include "control_tlv320.h"
AudioControlTLV320    audioHardware;    //controller for the Teensy Audio Board
#endif
AudioSynthWaveformSine  testSignal;          //use to generate test tone as input
AudioInputI2S           i2s_in;          //Digital audio *from* the Teensy Audio Board ADC.  Sends Int16.  Stereo.
AudioOutputI2S          i2s_out;        //Digital audio *to* the Teensy Audio Board DAC.  Expects Int16.  Stereo

//create audio objects
AudioConvert_I16toF32   int2Float1;     //Converts Int16 to Float.  See class in AudioStream_F32.h
AudioEffectGain_F32     preGain;
#define NCHAN 8
AudioFilterFIR_F32      firFilt[NCHAN];
AudioEffectCompressor_F32 compPerChan[NCHAN];
AudioMixer4_F32           mixer1, mixer2, mixer3; //mix floating point data
AudioConvert_F32toI16   float2Int1;     //Converts Float to Int16.  See class in AudioStream_F32.h

//make the audio connections
#if (USE_TEST_TONE_INPUT == 1)
  //use test tone as audio input
  AudioConnection         patchCord1(testSignal, 0, int2Float1, 0);    //connect the Left input to the Left Int->Float converter
#else
  //use real audio input (microphones or line-in)
  AudioConnection         patchCord1(i2s_in, 0, int2Float1, 0);    //connect the Left input to the Left Int->Float converter
#endif
AudioConnection_F32     patchCord2(int2Float1, 0, preGain, 0);
AudioConnection_F32     patchCord11(preGain, 0, firFilt[0], 0);
AudioConnection_F32     patchCord12(preGain, 0, firFilt[1], 0);
AudioConnection_F32     patchCord13(preGain, 0, firFilt[2], 0);
AudioConnection_F32     patchCord14(preGain, 0, firFilt[3], 0);
AudioConnection_F32     patchCord15(preGain, 0, firFilt[4], 0);
AudioConnection_F32     patchCord16(preGain, 0, firFilt[5], 0);
AudioConnection_F32     patchCord17(preGain, 0, firFilt[6], 0);
AudioConnection_F32     patchCord18(preGain, 0, firFilt[7], 0);

#if (USE_PER_CHANNEL_AGC == 1)
  AudioConnection_F32     patchCord21(firFilt[0], 0, compPerChan[0], 0);
  AudioConnection_F32     patchCord22(firFilt[1], 0, compPerChan[1], 0);
  AudioConnection_F32     patchCord23(firFilt[2], 0, compPerChan[2], 0);
  AudioConnection_F32     patchCord24(firFilt[3], 0, compPerChan[3], 0);
  AudioConnection_F32     patchCord25(firFilt[4], 0, compPerChan[4], 0);
  AudioConnection_F32     patchCord26(firFilt[5], 0, compPerChan[5], 0);
  AudioConnection_F32     patchCord27(firFilt[6], 0, compPerChan[6], 0);
  AudioConnection_F32     patchCord28(firFilt[7], 0, compPerChan[7], 0);
  AudioConnection_F32     patchCord31(compPerChan[0], 0, mixer1, 0);
  AudioConnection_F32     patchCord32(compPerChan[1], 0, mixer1, 1);
  AudioConnection_F32     patchCord33(compPerChan[2], 0, mixer1, 2);
  AudioConnection_F32     patchCord34(compPerChan[3], 0, mixer1, 3);
  AudioConnection_F32     patchCord35(compPerChan[4], 0, mixer2, 0);
  AudioConnection_F32     patchCord36(compPerChan[5], 0, mixer2, 1);
  AudioConnection_F32     patchCord37(compPerChan[6], 0, mixer2, 2);
  AudioConnection_F32     patchCord38(compPerChan[7], 0, mixer2, 3);
#else
  AudioConnection_F32     patchCord21(firFilt[0], 0, mixer1, 0);
  AudioConnection_F32     patchCord22(firFilt[1], 0, mixer1, 1);
  AudioConnection_F32     patchCord23(firFilt[2], 0, mixer1, 2);
  AudioConnection_F32     patchCord24(firFilt[3], 0, mixer1, 3);
  AudioConnection_F32     patchCord25(firFilt[4], 0, mixer2, 0);
  AudioConnection_F32     patchCord26(firFilt[5], 0, mixer2, 1);
  AudioConnection_F32     patchCord27(firFilt[6], 0, mixer2, 2);
  AudioConnection_F32     patchCord28(firFilt[7], 0, mixer2, 3);
#endif

AudioConnection_F32     patchCord41(mixer1, 0, mixer3, 0);
AudioConnection_F32     patchCord42(mixer2, 0, mixer3, 1);
AudioConnection_F32     patchCord43(mixer3, 0, float2Int1, 0);    //Left.  makes Float connections between objects
AudioConnection         patchCord51(float2Int1, 0, i2s_out, 0);  //connect the Left float processor to the Left output
AudioConnection         patchCord52(float2Int1, 0, i2s_out, 1);  //connect the Right float processor to the Right output


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
  //audioHardware.inputSelect(TYMPAN_INPUT_MIC_JACK); // use the microphone jack
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones // default

  //choose mic bias (if using mics on input jack)
  int myBiasLevel = TYMPAN_MIC_BIAS_2_5;  //choices: TYMPAN_MIC_BIAS_2_5, TYMPAN_MIC_BIAS_1_7, TYMPAN_MIC_BIAS_1_25, TYMPAN_MIC_BIAS_VSUPPLY
  audioHardware.setMicBias(myBiasLevel); // set mic bias to 2.5 // default

  //set volumes
  audioHardware.volume(0);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  audioHardware.micGain(10); // set MICPGA volume, 0-47.5dB in 0.5dB setps
#endif
}

#define N_FIR 64
#include "filtCoeff_64NFIR_8chan_fs24000Hz.h"
void setupAudioProcessing(void) {
  //set the pre-gain
  preGain.setGain_dB(30.0f);

  //set the filter coefficients
  firFilt[0].begin(firCoeff0, N_FIR);
  firFilt[1].begin(firCoeff1, N_FIR);
  firFilt[2].begin(firCoeff2, N_FIR);
  firFilt[3].begin(firCoeff3, N_FIR);
  firFilt[4].begin(firCoeff4, N_FIR);
  firFilt[5].begin(firCoeff5, N_FIR);
  firFilt[6].begin(firCoeff6, N_FIR);
  firFilt[7].begin(firCoeff7, N_FIR);

  //setup compressors
  boolean use_HP_filter = false;
  float32_t knee_dBFS = -50.f, comp_ratio = 3.0f;
  float32_t attack_sec = 0.050f, release_sec = 0.2f;

  //setup the compressors
  for (int Ichan = 0; Ichan < NCHAN; Ichan++) {
    compPerChan[Ichan].enableHPFilter(use_HP_filter);
    compPerChan[Ichan].setThresh_dBFS(knee_dBFS);
    compPerChan[Ichan].setCompressionRatio(comp_ratio);
    compPerChan[Ichan].setAttack_sec(attack_sec, CUSTOM_SAMPLE_RATE);
    compPerChan[Ichan].setRelease_sec(release_sec, CUSTOM_SAMPLE_RATE);
  }

  // set the gains on the mixers
  mixer1.gain(0,0.1);
  mixer1.gain(1,0.1);
  mixer1.gain(2,0.1);
  mixer1.gain(3,0.1);
  mixer2.gain(0,0.3);
  mixer2.gain(1,0.5);
  mixer2.gain(2,0.8);
  mixer2.gain(3,1.0);
  
  mixer3.gain(0,1.0);
  mixer3.gain(1,1.0);
}

// define the setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //open the USB serial link to enable debugging messages
  delay(500);             //give the computer's USB serial system a moment to catch up.
  Serial.println("GenericHearingAid: setup()...");
  Serial.print("Global: F_CPU: "); Serial.println(F_CPU);
  Serial.print("Global: F_PLL: "); Serial.println(F_PLL);
  Serial.print("Global: AUDIO_SAMPLE_RATE: "); Serial.println(AUDIO_SAMPLE_RATE);
  Serial.print("Global: AUDIO_BLOCK_SAMPLES: "); Serial.println(AUDIO_BLOCK_SAMPLES);

  // Audio connections require memory
  AudioMemory(30);      //allocate Int16 audio data blocks
  AudioMemory_F32(30);  //allocate Float32 audio data blocks

  //change the sample rate...this is required for any sample rate other than 44100...WEA to fix.
  setI2SFreq((int)AUDIO_SAMPLE_RATE); //set the sample rate for the Audio Card (the rest of the library doesn't know, though)

  // Enable the audio shield, select input, and enable output
  setupAudioHardware();
  Serial.println("Audio Hardware Setup Complete.");

  //setup filters and mixers
  setupAudioProcessing();

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

  //setup sine wave as test signal
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

  //print the compressor state
  printCompressorState(millis(),&Serial);
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

      //Serial.print("Sending new value to my algorithms: ");
      //Serial.println(effect1.setUserParameter(val));   //effect2.setUserParameter(val);
      if (USE_TYMPAN == 1) val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)

      #if USE_TEST_TONE_INPUT==1
            float freq = 700.f + 200.f * ((val - 0.5) * 2.0); //change tone 700Hz +/- 200 Hz
            Serial.print("Changing tone frequency to = "); Serial.println(freq);
            testSignal.frequency(freq);
      #else
        #if USE_TYMPAN == 1
              float vol_dB = 0.f + 15.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 15 dB
              Serial.print("Changing output volume frequency to = "); Serial.print(vol_dB); Serial.println(" dB");
              audioHardware.volume(vol_dB);
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
  static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("CPU Cur/Peak: ");
    Serial.print(AudioProcessorUsage());
    Serial.print("%/");
    Serial.print(AudioProcessorUsageMax());
    Serial.print("%,   ");
    Serial.print("MEMORY Int16 Cur/Peak: ");
    Serial.print(AudioMemoryUsage());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax());
    Serial.print(",   ");
    Serial.print("MEMORY Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void printCompressorState(unsigned long curTime_millis, Stream *s) {
  static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

      s->print("Current Compressor: Pre-Gain (dB) = ");
      s->print(preGain.getGain_dB());
      s->print(", Dynamic Gain L/R (dB) = ");
      for (int Ichan = 0; Ichan < NCHAN; Ichan++ ) {
        s->print(compPerChan[Ichan].getCurrentGain_dB());
        s->print(", ");
      }
      s->println();

      lastUpdate_millis = curTime_millis; //we will use this value the next time around.
    }
};

//Here's the function to change the sample rate of the system (via changing the clocking of the I2S bus)
//https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate?p=121365&viewfull=1#post121365
float setI2SFreq(int freq) {
  typedef struct {
    uint8_t mult;
    uint16_t div;
  } __attribute__((__packed__)) tmclk;

  const int numfreqs = 15;
  const int samplefreqs[numfreqs] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44117.64706 , 48000, 88200, 44117.64706 * 2, 96000, 176400, 44117.64706 * 4, 192000};

#if (F_PLL==16000000)
  const tmclk clkArr[numfreqs] = {{16, 125}, {148, 839}, {32, 125}, {145, 411}, {48, 125}, {64, 125}, {151, 214}, {12, 17}, {96, 125}, {151, 107}, {24, 17}, {192, 125}, {127, 45}, {48, 17}, {255, 83} };
#elif (F_PLL==72000000)
  const tmclk clkArr[numfreqs] = {{32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {32, 375}, {128, 1125}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375}, {249, 397}, {32, 51}, {185, 271} };
#elif (F_PLL==96000000)
  const tmclk clkArr[numfreqs] = {{8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {8, 125},  {32, 375}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125}, {151, 321}, {8, 17}, {64, 125} };
#elif (F_PLL==120000000)
  const tmclk clkArr[numfreqs] = {{32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {32, 625},  {128, 1875}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625}, {178, 473}, {32, 85}, {145, 354} };
#elif (F_PLL==144000000)
  const tmclk clkArr[numfreqs] = {{16, 1125}, {49, 2500}, {32, 1125}, {49, 1250}, {16, 375},  {64, 1125}, {49, 625}, {4, 51}, {32, 375}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375} };
#elif (F_PLL==180000000)
  const tmclk clkArr[numfreqs] = {{46, 4043}, {49, 3125}, {73, 3208}, {98, 3125}, {37, 1084},  {183, 4021}, {196, 3125}, {16, 255}, {128, 1875}, {107, 853}, {32, 255}, {219, 1604}, {214, 853}, {64, 255}, {219, 802} };
#elif (F_PLL==192000000)
  const tmclk clkArr[numfreqs] = {{4, 375}, {37, 2517}, {8, 375}, {73, 2483}, {4, 125}, {16, 375}, {147, 2500}, {1, 17}, {8, 125}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125} };
#elif (F_PLL==216000000)
  const tmclk clkArr[numfreqs] = {{32, 3375}, {49, 3750}, {64, 3375}, {49, 1875}, {32, 1125},  {128, 3375}, {98, 1875}, {8, 153}, {64, 1125}, {196, 1875}, {16, 153}, {128, 1125}, {226, 1081}, {32, 153}, {147, 646} };
#elif (F_PLL==240000000)
  const tmclk clkArr[numfreqs] = {{16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {16, 625}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625} };
#endif

  for (int f = 0; f < numfreqs; f++) {
    if ( freq == samplefreqs[f] ) {
      while (I2S0_MCR & I2S_MCR_DUF) ;
      I2S0_MDR = I2S_MDR_FRACT((clkArr[f].mult - 1)) | I2S_MDR_DIVIDE((clkArr[f].div - 1));
      return (float)(F_PLL / 256 * clkArr[f].mult / clkArr[f].div);
    }
  }
  return 0.0f;
}
