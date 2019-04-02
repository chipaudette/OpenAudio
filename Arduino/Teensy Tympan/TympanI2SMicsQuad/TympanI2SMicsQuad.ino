// Quad channel input/output test
//
// Created: Chip Audette (OpenAudio), December 2018
//
// This example code is in the public domain.

// This test was done with two Tympan RevC boards and one Teensy 3.6

// The first Tympan audio board is hooked up as normal (ie, to I2S_RXD0 and I2S_TXD0)
// The second Tympan audio board is hooked up differently (ie, to I2S_RXD1 and I2S_TXD1)
//
// See: https://www.sparkfun.com/news/2055
// See: https://forum.pjrc.com/threads/41371-Quad-channel-output-on-Teensy-3-6
// See: https://forum.pjrc.com/threads/29373-Bit-bang-multiple-I2S-inputs-simultaneously?p=79606&viewfull=1#post79606
//
// On Teensy 3.6, RXD1 is Teensy 3.6 Pin 38 (PTC11), and TXD1 is Teensy 3.6 Pin 15 (PTC0).
// The Teensy Audio code for the I2SQuad objects follows to the correct pins for the second
// Tympan board.  Yay.

#include <Audio.h>   //yes, I'm going to use the Teensy Audio library
#include <control_tlv320aic3206.h>  //from the Tympan library, I'm including just this one header file.
#include "AudioEffectGain.h"

// Define audio objects...notice that I'm using the Teensy Audio library, not the Tympan library
AudioInputI2SQuad        i2s_quad_in;    //This is the Teensy Audio library's built-in 4-channel I2S class        
//AudioSynthNoisePink      pink1,pink2;        
//AudioSynthWaveformSine   sine1,sine2;    
AudioEffectGain         gain1, gain2;      
AudioOutputI2SQuad       i2s_quad_out;   //This is the Teensy Audio library's built-in 4-channel I2S class   

//setup the first Tympan using the default settings
AudioControlTLV320AIC3206     tympan1;  //using I2C bus SCL0/SDA0

#define POT_PIN 39

//setup the second Tympan using the pins just for the second AIC
//define AIC_ALT_REST_PIN 20
//define AIC_ALT_I2C_BUS 2
//AudioControlTLV320AIC3206     tympan2(AIC_ALT_REST_PIN,AIC_ALT_I2C_BUS);  //second Tympan! using I2C bus SCL2/SDA2

//define the audio connections (again, using the Teensy Audio library classes, not Tympan library)
#if 0
  //play synthetic sounds...tests only the *output* of the 4-channel system
  AudioConnection          patchCord1(sine1, 0, i2s_quad_out, 0); //sine on left, Tympan1.  Freq is set later.
  AudioConnection          patchCord2(pink1, 0, i2s_quad_out, 1); //pink noise on right, Tympan1
  AudioConnection          patchCord3(pink2, 0, i2s_quad_out, 2); //pink noise on right, Tympan2
  AudioConnection          patchCord4(sine2, 0, i2s_quad_out, 3); //sine on left, Tympan2.  Freq is set later.
#else
  //connect the pink input jack (as line-in, not mic-in) to the output...test both input and output
  AudioConnection          patchCord1(i2s_quad_in, 0, i2s_quad_out, 2); //ignore
  AudioConnection          patchCord2(i2s_quad_in, 1, i2s_quad_out, 3); //ignore
  
  AudioConnection          patchCord3(i2s_quad_in, 2, gain1, 0); //connect to first AIC's outputs
  AudioConnection          patchCord4(i2s_quad_in, 3, gain2, 0); //connect to second AIC's outputs
  AudioConnection          patchCord5(gain1, 0, i2s_quad_out, 0); //connect to first AIC's outputs
  AudioConnection          patchCord6(gain2, 0, i2s_quad_out, 1); //connect to second AIC's outputs
#endif

const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob

void setup() {
  Serial.begin(115200);  delay(500);
  Serial.println("AudioPassThru: Starting setup()...");
  Wire1.end(); //delete Wire1;

  //allocate the dynamic memory for audio processing blocks
  AudioMemory(50);

  //Enable the Tympan to start the audio flowing!
  tympan1.enable();
  //tympan2.enable();
  pinMode(POT_PIN,INPUT);

  //Choose the desired input
  //tympan1.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //tympan2.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //tympan1.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //tympan2.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  tympan1.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  //tympan2.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  tympan1.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  tympan1.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
  //tympan2.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  //tympan2.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //setup sound sources
//  sine1.frequency(250.0);sine1.amplitude(0.2);  //set frequency...this one is low
//  pink1.amplitude(0.2);
//  pink2.amplitude(0.2);
//  sine2.frequency(1900.0);sine2.amplitude(0.2);  //set frequency...this one is high
   
  Serial.println("Setup complete.");
}

bool setQuiet = false;
void loop() {

  //delay(1000);
  //Serial.println("playing...");
  
  #if 0
    //change output volume on just Tympan 2 to confirm that we can control one (and only one) Tympan
    setQuiet = !setQuiet;
    if (setQuiet) {
      //tympan2.volume_dB(vol_knob_gain_dB-10.0);
    } else {
      //tympan2.volume_dB(vol_knob_gain_dB);
    }
  #else
    //check the potentiometer
    servicePotentiometer(millis(),100); //service the potentiometer every 100 msec
  #endif
}

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
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      gain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      gain2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel gain processor
      Serial.print("servicePotentiometer: Digital Gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
