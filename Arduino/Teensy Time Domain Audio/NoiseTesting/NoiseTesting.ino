#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

class AudioFilterEmpty : public AudioStream
{
  public:
    AudioFilterEmpty(char *foo_txt) : AudioStream(1, inputQueueArray) {myName = foo_txt; }
    char* myName;
    void update(void)
    {
      audio_block_t *block;
      block = receiveWritable();
      if (!block) {
        //Serial.print("AudioFilterEmpty: ");
        //Serial.print(myName);
        //Serial.println(": Empty Block...");
        return;
      }
      transmit(block);
      release(block);
    }

  private:
    //int32_t definition[32];  // up to 4 cascaded biquads
    audio_block_t *inputQueueArray[1];
};

// GUItool: begin automatically generated code
//AudioSynthToneSweep   sine1;    //xy=204,361
AudioInputI2S            i2s1;           //xy=214,285
AudioOutputI2S           i2s2;           //xy=583,413
//AudioConnection          patchCord1(sine1, 0, filter1, 0);
#if 1
AudioFilterEmpty         filter1("Filter1");
AudioFilterEmpty         filter2("Filter2");
AudioConnection          patchCord1(i2s1, 0, filter1, 0);
AudioConnection          patchCord2(i2s1, 1, filter2, 0);
AudioConnection          patchCord3(filter1, 0, i2s2, 0);
AudioConnection          patchCord4(filter2, 0, i2s2, 1);
#else
AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
AudioConnection          patchCord2(i2s1, 1, i2s2, 1);
#endif
//AudioOutputUSB           usb1;           //xy=593,466
//AudioConnection          patchCord5(filter1, 0, usb1, 1);
//AudioConnection          patchCord6(filter2, 0, usb1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=392,522
// GUItool: end automatically generated code

#define POT_PIN A1  //potentiometer is tied to this pin
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;


void setup() {
  // Start the serial debugging
  Serial.begin(115200);
  delay(750);
  Serial.println("Teensy Aduio: NoiseTesting");

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(20);

  // Enable the audio shield and set the output volume.
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.45); //headphone volume
  sgtl5000_1.lineInLevel(11, 11); //max is 15, default is 5

  //setup the sine wave
  //sine1.amplitude(0.1);  //0 is off
  //sine1.frequency(2490.0);
  Serial.println("Finished With Setup");

  //start the tone sweep
  delay(1000);
  float ampx = 0.1;
  int f_lox = 200;
  int f_hix = 4000;
  // Length of time for the sweep in seconds
  float t_timex = 10;
  //if (!sine1.play(ampx, f_lox, f_hix, t_timex)) { Serial.println("AudioSynthToneSweep - begin failed");  while (1); }

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
