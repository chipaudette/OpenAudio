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
#include <math.h>

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

    void setGain(int g) { gain = min(g,1000); } //limit the gain to 60 dB
  private:
    audio_block_t *inputQueueArray[1];
    int gain=1; //default value
};


class AudioControlSGTL5000_Extended : public AudioControlSGTL5000
{
  public:
    AudioControlSGTL5000_Extended(void) {};
    bool micBiasEnable(void) {
      return micBiasEnable(3.0);
    }
    bool micBiasEnable(float volt) {
      uint16_t bias_resistor_setting = 1 << 8;  //2kOhm
      uint16_t  bias_voltage_setting = ((uint16_t)((volt - 1.25)/0.250 + 0.5)) << 4;
      return write(0x002A, bias_voltage_setting | bias_resistor_setting);
    }
     
};


// GUItool: begin automatically generated code
AudioControlSGTL5000_Extended     audioShield;     //xy=360,498
AudioInputI2S            i2s1;           //xy=233,191
AudioAnalyzePeak     peak_L;
AudioAnalyzePeak     peak_R;
AudioFilterBiquad        biquad1;        //xy=410,161
AudioFilterBiquad        biquad2;        //xy=408,208
AudioOutputI2S           i2s2;           //xy=612,188
AudioFilterGain          gain1;
AudioFilterGain          gain2;


#define PROCESSING_TYPE 1
#define DO_USB_OUT  1

AudioConnection patchCord101(i2s1, 0, peak_L, 0);  //use these in all configurations
AudioConnection patchCord102(i2s1, 1, peak_R, 0);  //use these in all configurations

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
    AudioConnection          patchCord13(gain2, 0, usb1, 1);
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

#define PEAKHOLD_SEC (1.0)
#define N_PEAKHOLD (((int)(PEAKHOLD_SEC*44100.0/AUDIO_BLOCK_SAMPLES))+1)
class PeakHold {
  public:
    PeakHold(void) {reset();}
    void addValue(float value) {
      data[ind_next_data++]=value;
      if (ind_next_data >= N_PEAKHOLD) ind_next_data=0;
    }
    float getMaxAbs(void) {
      float maxAbsVal = 0.0;
      for (int i=0; i<N_PEAKHOLD; i++) {
        if (fabs(data[i]) > maxAbsVal) maxAbsVal = fabs(data[i]);
      }
      return maxAbsVal;
    }
    float getMaxAbsDB(void) {
      float val = getMaxAbs();
      if (val == 0.0) {
        return -200.0;
      } else {
        return 20.0*log10(val);
      }
    }
    void reset(void) { for (int i=0; i<N_PEAKHOLD; i++) data[i]=0.0; }
  private:
    float data[N_PEAKHOLD]; //should be -1.0 to +1.0
    int ind_next_data=0;
};

float headphone_val = 0.8;
int lineIn_val = 10;
PeakHold peakHold[2];  //left and right
void setup() {
  // Start the serial debugging
  Serial.begin(115200);
  Serial1.begin(2*115200); //2*115200 for BT Classic, 9600 for adafruit bluefruit BLE
  delay(500);
  Serial.println("USB: Teensy Aduio: Gain and EQ");
  Serial1.println("BT: Teensy Aduio: Gain and EQ");
  delay(250);

 // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(20);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  Serial.print("USB: Headphone setting: "); Serial.println(headphone_val);
  Serial1.print("BT: Headphone setting: "); Serial1.println(headphone_val);
  audioShield.volume(headphone_val); //headphone volume (max linear is about 0.85)

  lowshelf.stage =  highshelf.stage+2;  //there are two lowshelf and two highshelf
  lowshelf.gain = 1.0/highshelf.gain; //invert the gain so that the lows drop as the highs increase
  #if PROCESSING_TYPE > 0
    configureMultipleHighShelfFilters(&highshelf);
    configureMultipleLowShelfFilters(&lowshelf);
  #endif

  Serial.print("USB: LineInLevel setting: "); Serial.println(lineIn_val);
  Serial1.print("BT: LineInLevel setting: "); Serial1.println(lineIn_val);
  audioShield.lineInLevel(lineIn_val,lineIn_val); //max is 15, default is 5
  audioShield.micBiasEnable(3.0); //set the mic bias voltage

  //let the ADC highpass filter do its thing
  audioShield.adcHighPassFilterDisable();  //reduce noise?  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
  Serial.println("USB: adcHighPassFilter Disabled.");
  Serial1.println("BT: adcHighPassFilter Disabled.");
  

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

void updateSignalTracking(void) {
  if (peak_L.available() && peak_R.available()) {
    //Serial.print("updateSignal: updating...");Serial.println(peak_L.read()*100.0);
    peakHold[0].addValue(peak_L.read());
    peakHold[1].addValue(peak_R.read());
  }    
}

long lastTime = millis();
boolean ADCHighPassEnabled = true;
int count=0;
unsigned long updatePeriod_millis = 500;
unsigned long lastUpdate_millis = 0;
unsigned long curTime_millis=0;
int prev_gain_setting = -1;  //negative value says to update the next time through
void loop() {
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  
  delay(5);
  updateSignalTracking();

  //update the GUI and the print statements?
  curTime_millis = millis();
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis=0;
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) {   
    //display peak hold
    Serial.print("USB "); Serial.print(count % 100); Serial.print(": Peak dBFS: "); Serial.print(peakHold[0].getMaxAbsDB());Serial.print(", "); Serial.println(peakHold[1].getMaxAbsDB());
    Serial1.print("BT "); Serial1.print(count % 100); Serial1.print(": Peak dBFS: "); Serial1.print(peakHold[0].getMaxAbsDB());Serial1.print(", "); Serial1.println(peakHold[1].getMaxAbsDB());

    //service the user interface
    #if PROCESSING_TYPE > 0 //skip it if there is no processing happening
      //read potentiometer
      float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
      float gain;
      float gain_dB;
      float max_gain_dB;
      int round_factor;
      int cur_gain_setting = 0;
      float all_gain_dB[4] = {0.0, 10.0, 20.0, 30.0};      
      count++;
  
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
          Serial.print("USB "); Serial.print(count); Serial.print(": Gain dB = "); Serial.println(20.0*log10(gain));
          Serial1.print("BT "); Serial1.print(count); Serial1.print(": Gain dB = "); Serial1.println(20.0*log10(gain));
          break;
       case 1:
          cur_gain_setting = 0;
          if (val > 0.825) {
            cur_gain_setting = 3;
          } else if (val > 0.5) {
            cur_gain_setting = 2;
          } else if (val > 0.2) {
            cur_gain_setting = 1;
          }
          if (cur_gain_setting != prev_gain_setting) {
            prev_gain_setting = cur_gain_setting;
            gain_dB = all_gain_dB[cur_gain_setting];
            gain = pow(10.0,gain_dB/20.0);
            gain1.setGain(gain); gain2.setGain(gain);
            Serial.print("USB "); Serial.print(count); Serial.print(": Gain dB = "); Serial.println(20.0*log10(gain));
            Serial1.print("BT "); Serial1.print(count); Serial1.print(": Gain dB = "); Serial1.println(20.0*log10(gain));
          }
          break;
        case 10:
          gain_dB = (float)(int)(val * 20.0); //scale 0.0 to 20.0 dB.  Truncate to a whole number of dB.
          highshelf.gain = pow(10.0,gain_dB/20.0);
          lowshelf.gain = 1.0; //disable the lowshelf
          Serial.print("USB "); Serial.print(count); Serial.print(": highshelf gain (dB): "); Serial.println(20.0*log10(highshelf.gain));
          Serial1.print("BT "); Serial1.print(count); Serial1.print(": highshelf gain (dB): "); Serial1.println(20.0*log10(highshelf.gain));
          break;
        case 11:
          highshelf.freq_Hz = (float)(100*((int)(0.01*(val * 12000.0 + 500.0))));  //round to the nearest 100
          lowshelf.gain = 1.0; //disable the lowshelf
          Serial.print("USB "); Serial.print(count); Serial.print(": highshelf freq (Hz): "); Serial.println(highshelf.freq_Hz);
          Serial1.print("BT "); Serial1.print(count); Serial1.print(": highshelf freq (Hz): "); Serial1.println(highshelf.freq_Hz);
          break;
        case 20:
          gain_dB = (float)(int)(val * 20.0); //scale 0.0 to 20.0 dB.  Truncate to a whole number of dB.
          highshelf.gain = pow(10.0,gain_dB/20.0);
          lowshelf.gain = 1.0/highshelf.gain;
          Serial.print("USB "); Serial.print(count); Serial.print(": highshelf/lowshelf gain (dB): "); Serial.println(-20.0*log10(lowshelf.gain));
          Serial1.print("BT "); Serial1.print(count); Serial1.print(": highshelf/lowshelf gain (dB): "); Serial1.println(-20.0*log10(lowshelf.gain));
          break;
        case 21:
          highshelf.freq_Hz = (float)(100*((int)(0.01*(val * 12000.0 + 500.0))));  //round to the nearest 100
          lowshelf.freq_Hz = highshelf.freq_Hz;
          Serial.print("USB "); Serial.print(count); Serial.print(": highshelf/lowhself freq (Hz): "); Serial.println(highshelf.freq_Hz);
          Serial1.print("BT "); Serial1.print(count); Serial1.print(": highshelf/lowhself freq (Hz): "); Serial1.println(highshelf.freq_Hz);
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

    lastUpdate_millis = millis();
  }
  
}
