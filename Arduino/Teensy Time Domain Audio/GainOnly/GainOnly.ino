/*
   GainOnly

   Created: Chip Audette, Nov 2016
   Purpose: Process audio by applying gain

   Uses Teensy Audio Adapter.
   Assumes microphones (or whatever) are attached to the LINE IN

   MIT License.  use at your own risk.

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
    void update(void) {
      audio_block_t *block;
      block = receiveWritable();
      if (!block) return;

      //apply the gain
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = gain * (block->data[i]);

      transmit(block);
      release(block);
    }
    void setGain(float g) {
      gain = min(g, 1000.);  //limit the gain to 60 dB
    }
  private:
    audio_block_t *inputQueueArray[1];
    float gain = 1.0; //default value
};

class GainManager {
  public:
    GainManager(void) {
      gain1_dB = 0;
      gain2_dB = 0;
    }
    float gain1_dB = 0.0;
    float gain2_dB = 0.0;
    float getTotalGain_dB(void) {
      return gain1_dB + gain2_dB;
    }
};


#define PEAKHOLD_SEC (1.0)
#define N_PEAKHOLD (((int)(PEAKHOLD_SEC*44100.0/AUDIO_BLOCK_SAMPLES))+1)
class PeakHold {
  public:
    PeakHold(void) {
      reset();
    }
    void addValue(float value) {
      data[ind_next_data++] = value;
      if (ind_next_data >= N_PEAKHOLD) ind_next_data = 0;
    }
    float getMaxAbs(void) {
      float maxAbsVal = 0.0;
      for (int i = 0; i < N_PEAKHOLD; i++) {
        if (fabs(data[i]) > maxAbsVal) maxAbsVal = fabs(data[i]);
      }
      return maxAbsVal;
    }
    float getMaxAbsDB(void) {
      float val = getMaxAbs();
      if (val == 0.0) {
        return -200.0;
      } else {
        return 20.0 * log10(val);
      }
    }
    void reset(void) {
      for (int i = 0; i < N_PEAKHOLD; i++) data[i] = 0.0;
    }
  private:
    float data[N_PEAKHOLD]; //should be -1.0 to +1.0
    int ind_next_data = 0;
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
      uint16_t  bias_voltage_setting = ((uint16_t)((volt - 1.25) / 0.250 + 0.5)) << 4;
      return write(0x002A, bias_voltage_setting | bias_resistor_setting);
    }
    bool micBiasDisable(void) {
      return write(0x002A, 0x00);
    }
};


// GUItool: begin automatically generated code
AudioControlSGTL5000_Extended     audioShield;     //xy=360,498
AudioInputI2S            i2s1;           //xy=233,191
AudioAnalyzePeak     peak_L;
AudioAnalyzePeak     peak_R;
AudioOutputI2S           i2s2;           //xy=612,188
AudioFilterGain          gain1;
AudioFilterGain          gain2;

//gain only
AudioConnection         patchCord1(i2s1, 0, gain1, 0);
AudioConnection         patchCord2(i2s1, 1, gain2, 0);
AudioConnection         patchCord101(i2s1, 0, peak_L, 0);  //use these in all configurations
AudioConnection         patchCord102(i2s1, 1, peak_R, 0);  //use these in all configurations
AudioConnection         patchCord10(gain1, 0, i2s2, 0);
AudioConnection         patchCord11(gain2, 0, i2s2, 1);


#define DO_USB_OUT  0
#if DO_USB_OUT
  AudioOutputUSB           usb1;
  AudioConnection          patchCord12(gain1, 0, usb1, 0);
  AudioConnection          patchCord13(gain2, 0, usb1, 1);
#endif

#define POT_PIN A1  //potentiometer is tied to this pin
const int myInput = AUDIO_INPUT_LINEIN;

float headphone_val = 0.8;
int lineIn_val = 10;
PeakHold peakHold[2];  //left and right
boolean ADCHighPassEnabled = false;
GainManager digitalGain;
const float targetMicBiasValue_V = 3.0;
float micBiasValue_V = targetMicBiasValue_V;
void setup() {
  // Start the serial debugging
  Serial.begin(115200);
  Serial1.begin(9600); //2*115200 for BT Classic, 9600 for adafruit bluefruit BLE
  delay(500);
  Serial.println("USB: Teensy Aduio: GainOnly");
  Serial1.println("BT: Teensy Aduio: GainOnly");
  delay(250);
  printHelp();

  // Allocate memory for the Teensy Audio Library functions
  AudioMemory(20);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  Serial.print("USB: Headphone setting: "); Serial.println(headphone_val);
  Serial1.print("BT: Headphone setting: "); Serial1.println(headphone_val);
  audioShield.volume(headphone_val); //headphone volume (max linear is about 0.85)

  Serial.print("USB: LineInLevel setting: "); Serial.println(lineIn_val);
  Serial1.print("BT: LineInLevel setting: "); Serial1.println(lineIn_val);
  audioShield.lineInLevel(lineIn_val, lineIn_val); //max is 15, default is 5

  if (micBiasValue_V > 0.1) {
    audioShield.micBiasEnable(micBiasValue_V); //set the mic bias voltage
    Serial.print("MicBias Enabled at ");Serial.println(micBiasValue_V);
    Serial1.print("MicBias Enabled at ");Serial1.println(micBiasValue_V);
  } else { 
    audioShield.micBiasDisable();
    Serial.println("MicBias Disabled.");
    Serial1.println("MicBias Disabled.");
  }

  //let the ADC highpass filter do its thing (or not)
  if (ADCHighPassEnabled) {
    audioShield.adcHighPassFilterEnable();  //default
  } else {
    audioShield.adcHighPassFilterDisable();  //reduce noise.  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
    Serial.println("USB: adcHighPassFilter Disabled.");
    Serial1.println("BT: adcHighPassFilter Disabled.");
  }
}


void updateSignalTracking(void) {
  if (peak_L.available() && peak_R.available()) {
    //Serial.print("updateSignal: updating...");Serial.println(peak_L.read()*100.0);
    peakHold[0].addValue(peak_L.read());
    peakHold[1].addValue(peak_R.read());
  }
}

float setDigitalGain_dB(float gain_dB, boolean flag_printGain) {
  //int gain = ((int)(pow(10.0, gain_dB / 20.0) + 0.5)); //round to nearest integer
  float gain = pow(10.0, gain_dB / 20.0); //round to nearest integer
  gain1.setGain(gain); gain2.setGain(gain);
  if (flag_printGain) {
    Serial.print("USB"); Serial.print(": Digital Gain dB = "); Serial.println(20.0 * log10(gain));
    Serial1.print("BT"); Serial1.print(": Digital Gain dB = "); Serial1.println(20.0 * log10(gain)); 
  }
  return gain_dB;
}


int count = 0;
unsigned long updatePeriod_millis = 1000;
unsigned long lastUpdate_millis = 0;
unsigned long curTime_millis = 0;
int prev_gain_setting = -1;  //negative value says to update the next time through
void loop() {
  serviceSerial();

  //delay(5);
  asm(" WFI");  //"wait for interrupt".  saves power.  stays on this line until an interrupt is issued anywhere in the system.  The audio library (secretly) services an interrupt every 128 samples, so this is an easy way to save power.
  updateSignalTracking();

  //update the GUI and the print statements?
  curTime_millis = millis();
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0;
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) {
    //display peak hold
    Serial.print("USB "); Serial.print(count % 100); Serial.print(": Peak dBFS: "); Serial.print(peakHold[0].getMaxAbsDB()); Serial.print(", "); Serial.println(peakHold[1].getMaxAbsDB());
    Serial1.print("BT "); Serial1.print(count % 100); Serial1.print(": Peak dBFS: "); Serial1.print(peakHold[0].getMaxAbsDB()); Serial1.print(", "); Serial1.println(peakHold[1].getMaxAbsDB());

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    //float gain;
    float gain_dB;
    float max_gain_dB;
    int round_factor;
    int cur_gain_setting = 0;
    float all_gain_dB[4] = {0.0, 10.0, 20.0, 30.0};
    count++;

    //decide what to do with the pot value
    int control_type = 1;
    switch (control_type) {
      case 0:
        max_gain_dB = 60.0;
        gain_dB = (float)(int)(val * max_gain_dB);  //scale 0.0 to 60.0 dB.  Truncate to a whole number of dB.
        round_factor = 3;  //round to this nearest value
        gain_dB = round_factor * (int)((float)gain_dB / ((float)round_factor) + 0.5); //round to nearest value (reduce chatter        
        digitalGain.gain1_dB = gain_dB;
        setDigitalGain_dB(digitalGain.getTotalGain_dB(),true);
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
          digitalGain.gain1_dB = gain_dB;
          setDigitalGain_dB(digitalGain.getTotalGain_dB(),true);
        }
        break;
    };
    
    lastUpdate_millis = millis();
  }

} //end loop()

void actUponCharCommand(char inChar) {
  switch (inChar) {
    case 'g':
      changeGain2_dB(1.0);
      break;
    case 'f':
      changeGain2_dB(-1.0);
      break;
    case 'b':
      toggleBias();
      break;
    case '?':
      printHelp();
      break;
  }
}

void serviceSerial(void) {
 while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    actUponCharCommand(inChar);
  }
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();

    //echo
    Serial.write(inChar);
    
    //act
    actUponCharCommand(inChar);
  }
}

void changeGain2_dB(float changeInGain_dB) {
  digitalGain.gain2_dB += changeInGain_dB;
  //Serial.print("DigitalGain2 = ");Serial.println(digitalGain.gain2_dB);
  //Serial.print("TotalGain = ");Serial.println(digitalGain.getTotalGain_dB());
  setDigitalGain_dB(digitalGain.getTotalGain_dB(), true);
}
void toggleBias(void) {
  if (micBiasValue_V < 1.0) {
    //it is currently off.  turn it on;
    micBiasValue_V = targetMicBiasValue_V;
    audioShield.micBiasEnable(micBiasValue_V); //set the mic bias voltage
    Serial.print("MicBias Enabled at ");Serial.println(micBiasValue_V);
    Serial1.print("MicBias Enabled at ");Serial1.println(micBiasValue_V);
  } else {
    //it is currently on.  turn it off;
    micBiasValue_V = 0.0;
    audioShield.micBiasDisable();
    Serial.println("MicBias Disabled.");
    Serial1.println("MicBias Disabled.");
  }
}
void printHelp(void) {
  Stream *s;
  for (int i=0; i < 2; i++) {
    if (i==0) {
      s = &Serial;
    } else {
      s = &Serial1;
    }
    s->println("Commands:");
    s->println("  g: increase digital gain");
    s->println("  f: decrease digital gain");
    s->println("  b: toggle mic bias");
    s->println("  ?: print this help message");
  }
}

