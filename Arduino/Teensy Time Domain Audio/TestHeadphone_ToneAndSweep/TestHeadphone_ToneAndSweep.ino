
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthToneSweep      tonesweep1;     //xy=86,141
AudioSynthWaveformSine   sine1;          //xy=93,84
AudioMixer4              mixer1;         //xy=274,85
AudioOutputI2S           i2s1;           //xy=458,92
AudioConnection          patchCord1(tonesweep1, 0, mixer1, 1);
AudioConnection          patchCord2(sine1, 0, mixer1, 0);
AudioConnection          patchCord3(mixer1, 0, i2s1, 0);
AudioConnection          patchCord4(mixer1, 0, i2s1, 1);

//AudioOutputUSB           usb1;           //xy=464,156
//AudioConnection          patchCord5(mixer1, 0, usb1, 0);
//AudioConnection          patchCord6(mixer1, 0, usb1, 1);

AudioControlSGTL5000     sgtl5000_1;     //xy=288,187
// GUItool: end automatically generated code


void setup() {
  // put your setup code here, to run once:

 delay(200);
  Serial.begin(115200);
  delay(200);
  
  AudioMemory(10);  //give Audio Library some memory

  //configure the Teensy Audio Board
  sgtl5000_1.enable();
  //sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.8); //headphone volume
  sgtl5000_1.adcHighPassFilterDisable();  //reduce noise?  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
  

  //configure the sine wave
  sine1.amplitude(0.0);


}


float level = 0.05;
float lowFreq = 20.;
float highFreq = 20000.;
float dur_sec = 5.0;
void loop() {

  //do tone at 1 kHz
  sine1.frequency(1000.0);
  sine1.amplitude(level);
  delay(dur_sec*1000);
 
  //do frequency sweep
  sine1.amplitude(0.0); //turn off sine
  tonesweep1.play(level, lowFreq, highFreq, dur_sec);
  while (tonesweep1.isPlaying()) {
    delay(10);
  }
}
