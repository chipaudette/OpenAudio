
//Use my custom version.  Only changes AUDIO_BLOCK_SAMPLES
#include "AudioStream.h"  //comment this out to return to the original version

//These are the includes from the Teensy Audio Library
#include <Audio.h>      //Teensy Audio Library
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Create audio objects
AudioControlSGTL5000    sgtl5000_1;    //controller for the Teensy Audio Board
AudioInputI2S           i2s_in;        //Digital audio *from* the Teensy Audio Board ADC.  Sends Int16.  Stereo.
AudioOutputI2S          i2s_out;       //Digital audio *to* the Teensy Audio Board DAC.  Expects Int16.  Stereo
AudioSynthWaveformSine   testSignal;          //xy=107,185

const int n_freqs = 3;
int freq_ind = 0;
float freqs_Hz[n_freqs] = {600.0, 700.0, 800.0};

// Make all of the audio connections
AudioConnection         patchCord1(testSignal, 0, i2s_out, 0);
AudioConnection         patchCord2(testSignal, 0, i2s_out, 1);
//AudioConnection         patchCord1(i2s_in, 0, i2s_out, 0);
//AudioConnection         patchCord2(i2s_in, 0, i2s_out, 1);


// define the setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //open the USB serial link to enable debugging messages
  delay(1000);             //give the computer's USB serial system a moment to catch up.
  Serial.print("ChangeBlockLength: AUDIO_BLOCK_SAMPLES: ");Serial.println(AUDIO_BLOCK_SAMPLES);

  // Audio connections require memory
  AudioMemory(10);      //allocate Int16 audio data blocks
 
  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();                   //start the audio board
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);       //choose line-in or mic-in
  sgtl5000_1.volume(0.8);      
 
  //setup sine wave as test signal
  testSignal.amplitude(0.01);
  testSignal.frequency(freqs_Hz[(freq_ind++) % n_freqs]);
  
} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
unsigned long lastUpdate_millis = 0;
void loop() {
  if ((millis() - lastUpdate_millis) > 2000) {
    Serial.println("Changing Frequency...");
    testSignal.frequency(freqs_Hz[(freq_ind++) % n_freqs]);
    lastUpdate_millis = millis();
  }
} //end loop()

