/*
   BasicGain

   Created: Chip Audette, Nov 2016
   Purpose: Process audio by applying gain.
            Demonstrates audio processing using floating point data type.

   Uses Teensy Audio Adapter.
   Assumes microphones (or whatever) are attached to the LINE IN

   MIT License.  use at your own risk.
*/

//These are the includes from the Teensy Audio Library
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "AudioStream_Float.h" //here is WEA custom code to extend audio library streams for floating point values
#include "AudioEffectGain.h" //here is the WEA custom audio processing module that does the gain

//create audio library objects for handling the audio
AudioControlSGTL5000    sgtl5000_1;    //controller for the Teensy Audio Board
AudioInputI2S           i2s1;          //Stereo.  Digital audio from the Teensy Audio Board ADC.  Sends Int16.
AudioOutputI2S          i2s2;          //Stereo.  Digital audio to the Teensy Audio Board DAC.  Expects Int16.
AudioConvert_I16toF32   int2Float1, int2Float2;    //Left and Right.  See AudioStream_Float.h
AudioConvert_F32toI16   float2Int1, float2Int2;    //Left and Right.  See AudioStream_Float.h
AudioEffectGain_F32     gain1, gain2;  //Left and Right.  

//AudioConnection     patchCord100(i2s1, 0, i2s2, 0);
//AudioConnection     patchCord101(i2s1, 1, i2s2, 1);

AudioConnection         patchCord1(i2s1, 0, int2Float1, 0);  //connect the Left input to the Left Int->Float converter
AudioConnection         patchCord2(i2s1, 1, int2Float2, 0);  //connect the Right input to the Right Int->Float converter
AudioConnection_F32     patchCord10(int2Float1, 0, gain1, 0);  //Left.  makes Float connections between objects
AudioConnection_F32     patchCord11(int2Float2, 0, gain2, 0);  //Right.  makes Float connections between objects
AudioConnection_F32     patchCord12(gain1, 0, float2Int1, 0);  //Left.  makes Float connections between objects
AudioConnection_F32     patchCord13(gain2, 0, float2Int2, 0);  //Right.  makes Float connections between objects
AudioConnection         patchCord20(float2Int1, 0, i2s2, 0); //connect the Left float processor to the Left output
AudioConnection         patchCord21(float2Int2, 0, i2s2, 1); //connect the Right float processor to the Right output


// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin


// define the setup() function, the function that is called once when
// the device is booting
void setup() {
  Serial.begin(115200);   //for debugging messages later
  delay(500);
  Serial.println("Teensy Hearing Aid: BasicGain_Float...");

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(20);
  AudioMemory_F32(20);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);  //line-in or mic-in
  sgtl5000_1.volume(0.8);      //0.0 to 1.0.  0.5 seems to be the usual default.
  sgtl5000_1.lineInLevel(10,10);  //0 to 15.  5 is the Teensy Audio Library's default
  sgtl5000_1.adcHighPassFilterDisable();  //reduce noise.  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831

  // setup other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

}


// define the loop() function, the function that is repeated over and over
// for the life of the device
unsigned long updatePeriod_millis = 100; //how many milliseconds between updating gain reading?
unsigned long lastUpdate_millis = 0;
unsigned long curTime_millis = 0;
int prev_gain_dB = 0;
void loop() {
  
  //has enough time passed to try updating the GUI?
  curTime_millis = millis(); //what time is it right now
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0;
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) {
    
    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = 0.1*(float)((int)(10.0*val + 0.5));  //quantize so that it doesn't chatter
        
    //compute desired digital gain
    const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
    float gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

    //if the gain is different than before, set the new gain value
    if (abs(gain_dB - prev_gain_dB) > 1.0) { //is it different than before
      gain1.setGain_dB(gain_dB);  //set the gain of the Left-channel gain processor
      gain2.setGain_dB(gain_dB);  //set the gain of the Right-channel gain processor
      Serial.print("Digital Gain dB = "); Serial.println(gain_dB); //print text to Serial port for debugging
      prev_gain_dB = gain_dB;
    }
 
    lastUpdate_millis = curTime_millis; //hold on to this time value
  } // end if

} //end loop();

