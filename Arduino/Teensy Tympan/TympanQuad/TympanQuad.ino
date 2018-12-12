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
// On Teensy 3.6, RXD1 is Teensy Pin XXX, and TXD1 is Teensy Pin XXX.  The Teensy Audio code
// for the I2SQuad objects follows to the correct pins for the second Tympan board.  Yay.

#include <Audio.h>
#include <control_tlv320aic3206.h>

// Define audio objects
AudioInputI2SQuad        i2s_quad_in;      //xy=228,103
//AudioInputI2S        i2s_quad_in;      //xy=228,103
AudioSynthNoisePink      pink1;          //xy=223,243
AudioSynthWaveformSine   sine1;          //xy=224,186
AudioSynthNoisePink      pink2;          //xy=223,243
AudioSynthWaveformSine   sine2;          //xy=224,186
AudioOutputI2SQuad       i2s_quad_out;      //xy=584,97
//AudioOutputI2S       i2s_quad_out;      //xy=584,97
AudioControlTLV320AIC3206     tympan1;
AudioControlTLV320AIC3206     tympan2;  

//define connections
#if 0
//play synthetic sounds
AudioConnection          patchCord1(sine1, 0, i2s_quad_out, 0);
AudioConnection          patchCord2(pink1, 0, i2s_quad_out, 1);
AudioConnection          patchCord3(pink2, 0, i2s_quad_out, 2);
AudioConnection          patchCord4(sine2, 0, i2s_quad_out, 3);
#else
//copy the input to the output...but jump between the two boards
AudioConnection          patchCord1(i2s_quad_in, 2, i2s_quad_out, 0);
AudioConnection          patchCord2(i2s_quad_in, 3, i2s_quad_out, 1);
AudioConnection          patchCord3(i2s_quad_in, 0, i2s_quad_out, 2);
AudioConnection          patchCord4(i2s_quad_in, 1, i2s_quad_out, 3);
#endif

const float input_gain_dB = 0.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob

void setup() {
  Serial.begin(115200);  delay(500);
  Serial.println("AudioPassThru: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory(50);

  //Enable the Tympan to start the audio flowing!
  tympan1.enable(); // activate AIC
  tympan2.enable(); // activate AIC

  //Choose the desired input
  //tympan1.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //tympan2.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //tympan1.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //tympan2.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  tympan1.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  tympan2.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  tympan1.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  tympan1.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
  tympan2.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  tympan2.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //setup sound sources
  sine1.frequency(250.0);sine1.amplitude(0.2);
  pink1.amplitude(0.2);
  pink2.amplitude(0.2);
  sine2.frequency(1900.0);sine2.amplitude(0.2);
   
  
  Serial.println("Setup complete.");
}

void loop() {
  delay(1000);
  Serial.println("playing...");
}

