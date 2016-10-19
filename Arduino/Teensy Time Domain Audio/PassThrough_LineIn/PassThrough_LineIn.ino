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
        return;
      }
      transmit(block);
      release(block);
    }

  private:
    audio_block_t *inputQueueArray[1];
};

AudioInputI2S            i2s1;         
AudioOutputI2S           i2s2;        
AudioFilterEmpty         filter1("Filter1");
AudioFilterEmpty         filter2("Filter2");
AudioConnection          patchCord1(i2s1, 0, filter1, 0);
AudioConnection          patchCord2(i2s1, 1, filter2, 0);
AudioConnection          patchCord3(filter1, 0, i2s2, 1);
AudioConnection          patchCord4(filter2, 0, i2s2, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=392,522

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("PassThrough_LineIn...");
  
  AudioMemory(20);
  delay(500);

  // Enable the audio shield and set the output volume.
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.45); //headphone volume
}

void loop() {
   delay(20);
}

