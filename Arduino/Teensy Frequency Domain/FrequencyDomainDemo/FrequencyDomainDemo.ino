// FrequencyDomainTrial
//
// Demonstrate audio procesing in the frequency domain.
//
// Approach:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins (LP filter?  BP filter?  Formant shift?)
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// Assumes the use of the Audio library from PJRC
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "AudioEffectFreqDomain.h"

//Note that the Teensy assumes that the audio block size is 128 samples at a sample rate of 44100 Hz.
//The block size (and maybe the sample rate?) is set in the "AudioStream.h" in the Teensy/core

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S          audioInput;         // audio shield: mic or line-in
AudioSynthWaveformSine sinewave;
//AudioAnalyzeFFT256     myFFT;
AudioEffectFreqDomain  audioEffectFD;
AudioOutputI2S         audioOutput;        // audio shield: headphones & line-out

// Connect either the live input or synthesized sine wave
//AudioConnection patchCord1(audioInput, 0, myFFT, 0);
//AudioConnection patchCord1(sinewave, 0, myFFT, 0);
AudioConnection patchCord1(sinewave, 0, audioEffectFD, 0);
AudioConnection patchCord2(audioEffectFD, 0, audioOutput, 0);

AudioControlSGTL5000 audioShield;

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  // Configure the window algorithm to use
  audioEffectFD.windowFunction(AudioWindowHanning256);
  //audioEffectFD.windowFunction(NULL);

  // Create a synthetic sine wave, for testing
  // To use this, edit the connections above
  sinewave.amplitude(0.8);
  sinewave.frequency(1034.007);
}

void loop() {
  float n;
  int i;

  delay(1);

//  if (myFFT.available()) {
//    // each time new FFT data is available
//    // print it all to the Arduino Serial Monitor
//    Serial.print("FFT: ");
//    for (i=0; i<40; i++) {
//      n = myFFT.read(i);
//      if (n >= 0.01) {
//        Serial.print(n);
//        Serial.print(" ");
//      } else {
//        Serial.print("  -  "); // don't print "0.00"
//      }
//    }
//    Serial.println();
//  }
}


