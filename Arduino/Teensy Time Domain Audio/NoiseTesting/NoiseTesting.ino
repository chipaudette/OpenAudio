#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioControlSGTL5000     sgtl5000_1;   
AudioInputI2S            i2s_in;          
AudioOutputI2S           i2s_out;         
AudioConnection          patchCord1(i2s_in, 0, i2s_out, 0);
AudioConnection          patchCord2(i2s_in, 1, i2s_out, 1);

#define DO_USB_OUT 1   //if activating this, be sure to choose USB->Audio under "Tools" in the Arduino IDE
#if DO_USB_OUT
  AudioOutputUSB           usb_out;   
  AudioConnection          patchCord20(i2s_in, 0, usb_out, 0);
  AudioConnection          patchCord21(i2s_in, 1, usb_out, 1);
#endif

//AudioSynthWaveformSine   sine1;  
//AudioConnection          patchCord10(sine1, 0, i2s_out, 1);
//#if DO_USB_OUT
//  AudioConnection          patchCord21(sine1, 0, usb_out, 1);
//#endif


#define POT_PIN A1  //potentiometer is tied to this pin
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

void setup() {
  // Start the serial debugging
  Serial.begin(115200);
  delay(500);
  Serial.println("Teensy Aduio: NoiseTesting");

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // Enable the audio shield and set the output volume.
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.5); //headphone volume
  sgtl5000_1.lineInLevel(5, 5); //max is 15, default is 5

  //sine1.amplitude(0.9);
  //sine1.frequency(555.0);

}

unsigned long last_time = millis();
void loop() {
  //read potentiometer
  float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0

  //decide what to do with the POT value
  int line_in_setting;
  switch (1) {
    case 0:
      line_in_setting = ((int)(15.0 * val + 0.5)); //the +0.5 is so that it rounds instead of truncates
      sgtl5000_1.lineInLevel(line_in_setting, line_in_setting); //max is 15, default is 5
      Serial.print("Line In Setting: "); Serial.println(line_in_setting);
      break;
    case 1:
      line_in_setting = 5;  //default
      if (val > 0.666) {
        line_in_setting = 15;
      } else if (val > 0.333) {
        line_in_setting = 10;   
      }
      sgtl5000_1.lineInLevel(line_in_setting, line_in_setting); //max is 15, default is 5
      //Serial.print("Line In Setting: "); Serial.println(line_in_setting);
      break;
  }
  delay(200);

  // print processor and memory usage
  if (0) {
    if (millis() - last_time >= 2000) { //every 2000 milliseconds
      Serial.print("Proc = ");
      Serial.print(AudioProcessorUsage());
      Serial.print(" (");
      Serial.print(AudioProcessorUsageMax());
      Serial.print("),  Mem = ");
      Serial.print(AudioMemoryUsage());
      Serial.print(" (");
      Serial.print(AudioMemoryUsageMax());
      Serial.println(")");
      last_time = millis();
    }
  }

}
