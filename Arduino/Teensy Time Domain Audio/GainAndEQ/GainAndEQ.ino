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

class AudioFilterGain : public AudioStream
{
  public:
    AudioFilterGain(void) : AudioStream(1, inputQueueArray) {}
    void update(void)
    {
      audio_block_t *block;
      block = receiveWritable();
      if (!block) return;

      //apply the gain
      for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i]=gain*(block->data[i]);
      
      transmit(block);
      release(block);
    }

    void setGain(int g) { gain = min(g,100); } //limit the gain to 40 dB
  private:
    audio_block_t *inputQueueArray[1];
    int gain=1; //default value
};

// GUItool: begin automatically generated code
AudioControlSGTL5000     audioShield;     //xy=360,498
AudioInputI2S            i2s1;           //xy=233,191
AudioFilterBiquad        biquad1;        //xy=410,161
AudioFilterBiquad        biquad2;        //xy=408,208
AudioOutputI2S           i2s2;           //xy=612,188
AudioFilterGain          gain1;
AudioFilterGain          gain2;


#define PROCESSING_TYPE 1
#define DO_USB_OUT  0

#if PROCESSING_TYPE == 0
  //pass through the audio
  AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
  AudioConnection          patchCord2(i2s1, 1, i2s2, 1);
  #if DO_USB_OUT
    AudioOutputUSB           usb1;  
    AudioConnection          patchCord12(i2s1, 0, usb1, 0);
    AudioConnection          patchCord13(i2s1, 1, usb1, 1);
  #endif
#elif PROCESSING_TYPE == 1
  //gain only
  AudioConnection         patchCord1(i2s1,0, gain1, 0);
  AudioConnection         patchCord2(i2s1,1, gain2, 0);
  AudioConnection          patchCord10(gain1, 0, i2s2, 0);
  AudioConnection          patchCord11(gain2, 0, i2s2, 1);
  #if DO_USB_OUT
    AudioOutputUSB           usb1;  
    AudioConnection          patchCord12(gain1, 0, usb1, 0);
    AudioConnection          patchCord13(gain2, 1, usb1, 1);
  #endif  
#elif PROCESSING_TYPE == 2
  //do gain and EQ processing
  AudioConnection          patchCord1(i2s1,0, gain1, 0);
  AudioConnection          patchCord2(i2s1,1, gain2, 0);  
  AudioConnection          patchCord3(gain1, 0, biquad1, 0);
  AudioConnection          patchCord4(gain2, 0, biquad2, 0);
  AudioConnection          patchCord10(biquad1, 0, i2s2, 0);
  AudioConnection          patchCord11(biquad2, 0, i2s2, 1);
  #if DO_USB_OUT
    AudioOutputUSB           usb1;           
    AudioConnection          patchCord12(biquad1, 0, usb1, 0);
    AudioConnection          patchCord13(biquad2, 0, usb1, 1);
  #endif
#endif



#define POT_PIN A1  //potentiometer is tied to this pin
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

float gain_dB = 20.0;
typedef struct {
  uint32_t  stage = 0;
  float freq_Hz = 6000.0;
  float gain = pow(10.0,gain_dB/20.0); //linear gain, not dB
  float slope = 1.0;
} biquad_shelf_t;
biquad_shelf_t highshelf;
biquad_shelf_t lowshelf;

void setup() {
  // Start the serial debugging
 Serial.begin(115200);
  delay(500);
  Serial.println("Teensy Aduio: Gain and EQ");
  delay(250);

 // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.8); //headphone volume, max linear is about 0.85
  audioShield.lineInLevel(5,5); //max is 15, default is 5
  audioShield.adcHighPassFilterDisable();  //reduce noise?  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831

//  Enable and configure the AGC that is part of the Audio shield
 // audioShield.audioPreProcessorEnable();
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

  lowshelf.stage =  highshelf.stage+2;  //there are two lowshelf and two highshelf
  lowshelf.gain = 1.0/highshelf.gain; //invert the gain so that the lows drop as the highs increase
  #if PROCESSING_TYPE > 0
    configureMultipleHighShelfFilters(&highshelf);
    configureMultipleLowShelfFilters(&lowshelf);
  #endif
}

void configureMultipleHighShelfFilters(biquad_shelf_t *myshelf) {
  biquad1.setHighShelf(myshelf->stage, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad2.setHighShelf(myshelf->stage, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad1.setHighShelf(myshelf->stage+1, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad2.setHighShelf(myshelf->stage+1, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
}

void configureMultipleLowShelfFilters(biquad_shelf_t *myshelf) {
  biquad1.setLowShelf(myshelf->stage, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad2.setLowShelf(myshelf->stage, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad1.setLowShelf(myshelf->stage+1, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
  biquad2.setLowShelf(myshelf->stage+1, myshelf->freq_Hz, myshelf->gain, myshelf->slope);
}

long lastTime = millis();
boolean ADCHighPassEnabled = true;
void loop() {
  #if PROCESSING_TYPE > 0 //skip it if there is no processing happening
    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    float gain;
    float gain_dB;
    float max_gain_dB;
    int round_factor;

    //decide what to do with the pot value
    int control_type = 21;
    #if PROCESSING_TYPE == 1
      control_type = 1;
    #endif
    switch (control_type) {
      case 0:
        max_gain_dB = 60.0;
        gain_dB = (float)(int)(val * max_gain_dB);  //scale 0.0 to 60.0 dB.  Truncate to a whole number of dB.
        round_factor = 3;  //round to this nearest value
        gain_dB = round_factor*(int)((float)gain_dB/((float)round_factor) + 0.5); //round to nearest value (reduce chatter)
        gain = ((int)(pow(10.0,gain_dB/20.0) + 0.5)); //round to nearest integer
        gain = max(gain,1);  //keep it to positive gain
        gain1.setGain(gain); gain2.setGain(gain);
        Serial.print("Gain: "); Serial.print(gain); Serial.print(", dB = "); Serial.println(20.0*log10(gain));
        break;
     case 1:
        gain_dB = 0.0;
        if (val > 0.8) {
          gain_dB = 30.0;
        } else if (val > 0.5) {
          gain_dB = 20.0;
        } else if (val > 0.2) {
          gain_dB = 10.0;
        }
        gain = pow(10.0,gain_dB/20.0);
        gain1.setGain(gain); gain2.setGain(gain);
        Serial.print("Quantized Gain: "); Serial.print(gain); Serial.print(", dB = "); Serial.println(20.0*log10(gain));
        break;
      case 10:
        gain_dB = (float)(int)(val * 20.0); //scale 0.0 to 20.0 dB.  Truncate to a whole number of dB.
        highshelf.gain = pow(10.0,gain_dB/20.0);
        lowshelf.gain = 1.0; //disable the lowshelf
        Serial.print("highshelf gain (dB): "); Serial.println(20.0*log10(highshelf.gain));
        break;
      case 11:
        highshelf.freq_Hz = (float)(100*((int)(0.01*(val * 12000.0 + 500.0))));  //round to the nearest 100
        lowshelf.gain = 1.0; //disable the lowshelf
        Serial.print("highshelf freq (Hz): "); Serial.println(highshelf.freq_Hz);
        break;
      case 20:
        gain_dB = (float)(int)(val * 20.0); //scale 0.0 to 20.0 dB.  Truncate to a whole number of dB.
        highshelf.gain = pow(10.0,gain_dB/20.0);
        lowshelf.gain = 1.0/highshelf.gain;
        Serial.print("highshelf/lowshelf gain (dB): "); Serial.println(-20.0*log10(lowshelf.gain));
        break;
      case 21:
        highshelf.freq_Hz = (float)(100*((int)(0.01*(val * 12000.0 + 500.0))));  //round to the nearest 100
        lowshelf.freq_Hz = highshelf.freq_Hz;
        Serial.print("highshelf/lowhself freq (Hz): "); Serial.println(highshelf.freq_Hz);
        break;
        
    };

    //re-configure the biquads (set the same for stereo)
    lowshelf.gain = 1.0/highshelf.gain; //invert the gain so that the lows drop as the highs increase
    configureMultipleHighShelfFilters(&highshelf);
    configureMultipleLowShelfFilters(&lowshelf);
  #endif 

//  if ((millis() - lastTime) > 3000) {
//    ADCHighPassEnabled = !ADCHighPassEnabled;
//    if (ADCHighPassEnabled) {
//      Serial.println("Enabling ADC HighPass Filter...");
//      audioShield.adcHighPassFilterEnable();
//    } else {
//      Serial.println("Disabling ADC HighPass Filter...");
//      audioShield.adcHighPassFilterDisable();
//    }
//    lastTime = millis();
//    
//  }

  delay(200);
}
