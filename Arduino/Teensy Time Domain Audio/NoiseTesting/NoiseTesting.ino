#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

class AudioControlSGTL500_wStates : public AudioControlSGTL5000 {
  public:
    AudioControlSGTL500_wStates(void) {
      inputSelect(AUDIO_INPUT_MIC); //also sets initial gain levels
      volume(volume_setting);
    }

    int getLineInLevel(void) { return line_in_setting[0]; }
    void getLineInLevel(int vals[]) { vals[0] = line_in_setting[0]; vals[1] = line_in_setting[1]; return; };
    int getMicGain(void) { return mic_gain_dB; };
    float getVolume(void) { return volume_setting; }
    
    bool inputSelect(int n) {
      bool val = AudioControlSGTL5000::inputSelect(n);
      if (n == AUDIO_INPUT_LINEIN) {
        micGain(mic_gain_dB);
      } else if (n == AUDIO_INPUT_LINEIN) {
        lineInLevel(line_in_setting[0],line_in_setting[1]);
      }
      return val;
    }
    bool lineInLevel(int n) { return lineInLevel(n,n); };
    bool lineInLevel(int n1, int n2) {
      line_in_setting[0] = n1; line_in_setting[1] = n2;
      return AudioControlSGTL5000::lineInLevel(line_in_setting[0],line_in_setting[1]);
    }
    bool micGain(int gain_dB) {
      mic_gain_dB = gain_dB;
      return AudioControlSGTL5000::micGain(mic_gain_dB);
    }
    bool volume(float vol) {
      volume_setting = vol;
      return AudioControlSGTL5000::volume(volume_setting);
    }
  private:
    int mic_gain_dB = 52;
    int line_in_setting[2] = {5, 5};
    float volume_setting = 0.8;
};

AudioControlSGTL500_wStates     sgtl5000_1;   
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

#define USE_MIC 0
#if USE_MIC > 0
  const int myInput = AUDIO_INPUT_MIC;
#else
  const int myInput = AUDIO_INPUT_LINEIN;
#endif

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
  sgtl5000_1.volume(0.8); //headphone volume (max linear is about 0.85)
  //sgtl5000_1.lineInLevel(5, 5); //max is 15, default is 5
  sgtl5000_1.adcHighPassFilterDisable();  //reduce noise?  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
  

  //sine1.amplitude(0.9);
  //sine1.frequency(555.0);

}

void setMicGainFromPot(float val) {
  int mic_gain_dB = 0; //default
  if (val > 0.75) {
    mic_gain_dB = 30;
  } else if (val > 0.5) {
    mic_gain_dB = 20;
  } else if (val > 0.25) {
    mic_gain_dB = 10;
  }
  sgtl5000_1.micGain(mic_gain_dB); //default is zero?
  Serial.print("Mic Setting, dB: "); Serial.println(mic_gain_dB);  
}

void setLineInLevelFromPot(float val) {
  int line_in_setting;
  switch (1) {
    case 0:
      line_in_setting = ((int)(15.0 * val + 0.5)); //the +0.5 is so that it rounds instead of truncates
      sgtl5000_1.lineInLevel(line_in_setting, line_in_setting); //max is 15, default is 5
      Serial.print("Line In Setting: "); Serial.println(line_in_setting);
      break;
    case 1:
      line_in_setting = 0;  //default
      if (val > 0.75) {
        line_in_setting = 15;
      } else if (val > 0.5) {
        line_in_setting = 10;
      } else if (val > 0.25) {
        line_in_setting = 5;   
      }
      sgtl5000_1.lineInLevel(line_in_setting, line_in_setting); //max is 15, default is 5
      Serial.print("Line In Setting: "); Serial.println(line_in_setting);
      break;
  }
}
unsigned long last_time = millis();
void loop() {
  //read potentiometer
  float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0

  //decide what to do with the POT value
  #if USE_MIC > 0
    setMicGainFromPot(val);
  #else
    setLineInLevelFromPot(val);
  #endif
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
