
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define USE_USB 0

// GUItool: begin automatically generated code
AudioControlSGTL5000     sgtl5000_1;     //xy=227.727294921875,127.72727966308594
AudioSynthWaveformSine   sine1;          //xy=120,217
AudioOutputI2S           i2s1;           //xy=329,212
AudioConnection          patchCord1(sine1, 0, i2s1, 0);
#if USE_USB == 1
AudioOutputUSB           usb1;           //xy=327,281
AudioConnection          patchCord2(sine1, 0, usb1, 0);
#endif
// GUItool: end automatically generated code

#define POT_PIN A1  //potentiometer is tied to this pin

void setup() {
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
  sine1.amplitude(0.95);
  sine1.frequency(1000);
}

void loop() {
 //read potentiometer
  float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0

  //set headphone volume
  val = ((int)(20.0*val + 0.5))/20.0; //round to nearest 0.1
  sgtl5000_1.volume(val); //headphone volume

  Serial.print("Headphone Volume = "); Serial.println(val);

  delay(200);
}
