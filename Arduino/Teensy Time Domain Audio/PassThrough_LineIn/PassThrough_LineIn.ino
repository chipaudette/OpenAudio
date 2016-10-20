/*
 * PassThrough_LineIn
 *     Test the ability to pass audio through the device from the LineIn input.
 * 
 * Created: Chip Audette, Oct 2016
 * 
 * Uses Teensy Audio Board.
 */
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioControlSGTL5000     sgtl5000_1;    
AudioInputI2S            i2s1;         
AudioOutputI2S           i2s2;

//simplest pass-through  (On Teensy 3.6: works at 96 MHz and 120MHz.  Not at 144/180/192/216/240MHz.
AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
AudioConnection          patchCord2(i2s1, 1, i2s2, 1);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Pass-Through Line-In to Headphone...");
  
  AudioMemory(20);
  delay(250);

  // Enable the audio shield and set the output volume.
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.45); //headphone volume
  //sgtl5000_1.lineInLevel(11, 11); //max is 15, default is 5
}

void loop() {
   delay(20);
}

