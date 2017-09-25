/*
 * TrebleBoost_toSD
 * 
 * Digitizes one channel of audio and processes it with TrebleBoost.  
 * Records the raw and processed audio to SD card.  Records for
 * fixed number of seconds and then stops recording.
 * 
 * Assumes use of Teensy 3.6 and Tympan Rev A or C.
 * 
 * Created: Chip Audette, OpenAudio, Sept 2017
 * 
 * License: MIT License, Use At Your Own Risk
 */

//here are the libraries that we need
#include "MyWriteAudioToSD.h"
#include <Tympan_Library.h>  //AudioControlTLV320AIC3206 lives here

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
AudioControlTLV320AIC3206 audioHardware;
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC. 
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.
AudioEffectGain_F32       gain1;                      //Applies digital gain to audio data. 
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC. 
AudioRecordQueue_F32      queueRaw(audio_settings);     //gives access to audio data (will use for SD card)
AudioRecordQueue_F32      queueProc(audio_settings);     //gives access to audio data (will use for SD card)

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect Left input (Raw) to HP filter
AudioConnection_F32       patchCord2(hp_filt1, 0, gain1, 0);    //connect HP filter to Gain
AudioConnection_F32       patchCord3(gain1, 0, i2s_out, 0);    //connect Gain output to Left output
AudioConnection_F32       patchCord4(gain1, 0, i2s_out, 1);    //connect Gain output to Right output
AudioConnection_F32       patchcord6(i2s_in, 0, queueRaw, 0);  //connect Raw audio to queue (to enable SD
AudioConnection_F32       patchcord7(gain1, 0, queueProc, 0);  //connect Processed audio to queue (to enable SD

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// Create variables to decide how long to record to SD
MyWriteAudioToSD my_SD_writer;
unsigned long started_millis = 0;             //note when (time) recording started
unsigned long desired_duration_millis = 5000;  //record for XXXX milliseconds

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("TrebleBoost: Starting setup()...");
  
  //allocate the audio memory
  AudioMemory(10); AudioMemory_F32(30,audio_settings); //allocate both kinds of memory
  
  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC
  
  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the highpassfilter
  float cutoff_Hz = 1000.f;  //frequencies below this will be attenuated
  Serial.print("Highpass filter cutoff at ");Serial.print(cutoff_Hz);Serial.println(" Hz");
  hp_filt1.setHighpass(0, cutoff_Hz); //biquad IIR filter for higpass
  
  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT
  
  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  //setup SD card and start recording
  my_SD_writer.setup();
  char *fname = "RECORD.RAW";
  if (my_SD_writer.open(fname)) {
    Serial.print("Opened "); Serial.print(fname); Serial.println(" on SD for writing.");
  } else {
    Serial.print("Failed to open "); Serial.print(fname); Serial.println(" on SD for writing.");
  }
  queueRaw.begin(); queueProc.begin();
  started_millis = millis();
  
  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //service the SD recording
  serviceSD();

  //check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  printCPUandMemory(millis(),3000); //print every 3000 msec 

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
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      gain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      Serial.print("servicePotentiometer: Digital Gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("printCPUandMemory: ");
    Serial.print("CPU Cur/Peak: ");
    Serial.print(audio_settings.processorUsage());
    //Serial.print(AudioProcessorUsage()); //if not using AudioSettings_F32
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
    //Serial.print(AudioProcessorUsageMax());  //if not using AudioSettings_F32
    Serial.print("%,   ");
    Serial.print("Dyn MEM Int16 Cur/Peak: ");
    Serial.print(AudioMemoryUsage());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax());
    Serial.print(",   ");
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void serviceSD(void) {
  if (my_SD_writer.isFileOpen()) {
    //if audio data is ready, write it to SD
    if ((queueRaw.available()) && (queueProc.available())) {
      my_SD_writer.writeF32AsInt16(queueRaw.readBuffer(),queueProc.readBuffer(),audio_block_samples);
      queueRaw.freeBuffer(); queueProc.freeBuffer();
    }
  
    //check to see if time has expired (so that we close the file)
    unsigned long current_recording_millis = millis() - started_millis;
    if (current_recording_millis >= desired_duration_millis) { 
      Serial.println("Closing SD File...");
      my_SD_writer.close();
    }
  }
}

