/*
*   ChangeBitDepth
*
*   Created: Chip Audette, Apr 2017
*   Purpose: Change the bit depth of the system
*
*   Uses Tympan Audio Adapter.
*   Blue potentiometer adjusts the frequency of the tone
*   
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Audio.h>           //include the Teensy Audio Library
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44117.64706 * 2 ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
const int bit_depth = 32;
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples, bit_depth);

//create audio library objects for handling the audio
AudioControlTLV320AIC3206 audioHardware;
//AudioInputI2S_32bit_F32         i2s_in(audio_settings);   //Digital audio *from* the Tympan AIC. 
AudioSynthWaveformSine_F32  sine1(audio_settings);

#if 0
  //original 16-bit version
  AudioOutputI2S_F32        i2s_out(audio_settings);  //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
#else
  //new 32-bit version
  AudioOutputI2S_32bit_F32        i2s_out(audio_settings);  //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
#endif

//Make all of the audio connections
AudioConnection_F32       patchCord11(sine1, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(sine1, 0, i2s_out, 1);  //connect the Right gain to the Right output


//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("ChangeBitDepth (Tone): starting setup()...");

  //hard reset the AIC
  Serial.println("Hardware reset of AIC...");
  pinMode(21,OUTPUT); 
  digitalWrite(21,HIGH);delay(100); //not reset
  digitalWrite(21,LOW);delay(100);  //reset
  digitalWrite(21,HIGH);delay(1000);//not reset
  
  //allocate the audio memory
  AudioMemory(10); AudioMemory_F32(200,audio_settings); //allocate both kinds of memory

  //Enable the Tympan to start the audio flowing!
  //audioHardware.setDataBitDepth(TYMPAN_DATA_BIT_DEPTH_32);
  Serial.println("AIC activating...");
  audioHardware.enable(); // activate AIC
  //Serial.println("Setting bit depth...");
  //audioHardware.setDataBitDepth(TYMPAN_DATA_BIT_DEPTH_32);
  

  //Choose the desired input
  //audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  sine1.frequency(1000.f);
  sine1.amplitude(0.03);
  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around
      val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)
      
      //choose the desired gain value based on the knob setting
      const float min_val = 200.0, max_val = 2000.0; //set desired range
      float new_value = min_val + (max_val - min_val)*val; //

      //command the new frequency setting
      sine1.frequency(new_value); 
      Serial.print("servicePotentiometer: Frequency (Hz) = "); Serial.println(new_value); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();

