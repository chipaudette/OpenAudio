// FrequencyDomainTrial
//
// Demonstrate audio procesing in the frequency domain.
//
// Created: Chip Audette  Sept-Oct 2016
//
// Approach:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins as desired (LP filter?  BP filter?  Formant shift?)
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// Assumes the use of the Audio library from PJRC
//
// This example code is in the public domain (MIT License)

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
//AudioInputI2S          audioInput;         // audio shield: mic or line-in
AudioSynthWaveformSine sinewave;
AudioEffectFreqDomain  audioEffectFD;

// Connect either the live input or synthesized sine wave
//AudioConnection patchCord1(sinewave, 0, audioOutput, 0);
AudioConnection patchCord1(sinewave, 0, audioEffectFD, 0);

//Send audio out to Teensy Audio Board
AudioOutputI2S         audioOutput;        // audio shield: headphones & line-out
AudioConnection patchCord2(audioEffectFD, 0, audioOutput, 0); //send to left channel
AudioConnection patchCord3(audioEffectFD, 0, audioOutput, 1); //also copy to right channel

//Send audio out over USB to the PC for PC-based audio recording?   must set Tools->USB Type->Audio
AudioOutputUSB  audioOutput_usb;          //must set Tools->USB Type->Audio
AudioConnection patchCord4(audioEffectFD, 0, audioOutput_usb, 0); //send to left channel
AudioConnection patchCord5(audioEffectFD, 0, audioOutput_usb, 1); //also copy to right channel


AudioControlSGTL5000 audioShield;

#define POT_PIN A1  //potentiometer is tied to this pin

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("FrequencyDomainDemo: beginning...");
  delay(500);
  Serial.print("    :AUDIO_BLOCK_SAMPLES = "); Serial.println(AUDIO_BLOCK_SAMPLES);
  delay(500);
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(24);

  // Configure the frequency-domain algorithm
  audioEffectFD.setup(); //do after AudioMemory();
  //audioEffectFD.windowFunction(AudioWindowHanning256);
  audioEffectFD.windowFunction(NULL);
  

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  // Create a synthetic sine wave, for testing
  // To use this, edit the connections above
  sinewave.amplitude(0.0625); //betweeon zero and one
  //sinewave.frequency(2*253.007);
  sinewave.frequency(((float)AUDIO_SAMPLE_RATE_EXACT)/256.0*3);
}



void loop() {
//  float n;
//  int i;

  int val = analogRead(POT_PIN);
  int new_shift = val / 20;
  audioEffectFD.setFreqShiftBins(new_shift);
  delay(125);
  
//  Serial.print("Pot = "); 
//  Serial.print(val);
//  Serial.print(", new shift = ");
//  Serial.print(new_shift);
//  Serial.println();

}


