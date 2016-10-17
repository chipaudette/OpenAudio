/*
 * GainAndEQ
 * 
 * Created: Chip Audette, Oct 2016
 * Purpose: Process audio by applying gain and frequency shaping (EQ)
 * 
 * Uses Teensy Audio Adapter.
 * Assumes microphones (or whatever) are attached to the LINE IN
 * 
 * MIT License.  use at your own risk.
 * 
 */



#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=233,191
AudioFilterBiquad        biquad2;        //xy=408,208
AudioFilterBiquad        biquad1;        //xy=410,161
AudioOutputUSB           usb1;           //xy=587,258
AudioOutputI2S           i2s2;           //xy=612,188
#if 1
//do EQ processing
AudioConnection          patchCord1(i2s1, 0, biquad1, 0);
AudioConnection          patchCord2(i2s1, 1, biquad2, 0);
AudioConnection          patchCord3(biquad2, 0, i2s2, 1);
AudioConnection          patchCord4(biquad2, 0, usb1, 1);
AudioConnection          patchCord5(biquad1, 0, i2s2, 0);
AudioConnection          patchCord6(biquad1, 0, usb1, 0);
#else
//pass through the audio
AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
AudioConnection          patchCord2(i2s1, 1, i2s2, 1);
AudioConnection          patchCord4(i2s1, 0, usb1, 0);
AudioConnection          patchCord6(i2s1, 1, usb1, 1);
#endif
AudioControlSGTL5000     audioShield;     //xy=360,498
// GUItool: end automatically generated code

#define POT_PIN A1  //potentiometer is tied to this pin
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

float gain_dB = 20.0;
typedef struct {
  int stage = 0;
  float freq_Hz = 2000.0;
  float gain = pow(10.0,gain_dB/20.0); //linear gain, not dB
  float slope = 1.0;
} biquad_shelf_t;
biquad_shelf_t highshelf;

void setup() {
  // Start the serial debugging
 Serial.begin(115200);
  delay(250);
  Serial.println("Teensy Aduio: Gain and EQ");
  delay(500);

 // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5); //headphone volume
  audioShield.lineInLevel(11,11); //max is 15, default is 5
 // audioShield.audioPreProcessorEnable();


//  Enable and configure the AGC that is part of the Audio shield
//  int maxGain = 2;  //can be 0 (0dB), 1 (6dB), or 2 (12 dB).
//  int response = 3;  //can be 0 (0ms), 1 (25ms), 2 (50 ms) or 3 (100ms);
//  int hardLimit = 0;  //1 or 0 for true or false
//  float threshold = -18.0; //dBFS
//  float attack = 5.0; // dB/sec
//  float decay = 5.0;  // dB/sec
//  audioShield.autoVolumeControl(maxGain, response, hardLimit,threshold,attack,decay);
//  audioShield.autoVolumeEnable();
  
//  audioShield.eqSelect(2);  //0=flat, 1=parametric, 2=bass/treble, 3=graphic EQ
//  float bass = -1.0;  //-1.0 to +1.0 is -11.75 dB to +12 dB
//  float treble = 1.0; //-1.0 to +1.0 is -11.75 dB to +12 dB
//  audioShield.eqBands(bass,treble);

  //configure the biquads (set the same for stereo)
  biquad1.setHighShelf(highshelf.stage,highshelf.freq_Hz,highshelf.gain,highshelf.slope);
  biquad2.setHighShelf(highshelf.stage,highshelf.freq_Hz,highshelf.gain,highshelf.slope);

}

void loop() {
  //read potentiometer
  float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0

  //decide what to do with the pot value
  switch (1) {
    case 0:
      gain_dB = (float)(int)(val * 20.0); //scale 0.0 to 20.0 dB.  Truncate to a whole number of dB.
      highshelf.gain = pow(10.0,gain_dB/20.0);
      Serial.print("highshelf gain (dB): "); Serial.println(20.0*log10(highshelf.gain));
      break;
    case 1:
      highshelf.freq_Hz = (float)(100*((int)(0.01*(val * 12000.0 + 500.0))));  //round to the nearest 100
      Serial.print("highshelf freq (Hz): "); Serial.println(highshelf.freq_Hz);
      break;
  };
  
  //re-configure the biquads (set the same for stereo)
  biquad1.setHighShelf(highshelf.stage,highshelf.freq_Hz,highshelf.gain,highshelf.slope);
  biquad2.setHighShelf(highshelf.stage,highshelf.freq_Hz,highshelf.gain,highshelf.slope);

  delay(200);
}
