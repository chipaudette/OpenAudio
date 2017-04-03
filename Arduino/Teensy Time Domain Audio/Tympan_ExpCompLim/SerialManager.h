
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//add in the algorithm whose gains we wish to set via this SerialManager
#include "AudioEffectExpCompLim.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectExpCompLim_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(int n, GainAlgorithm_t *gain_algs, 
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester)
      : N_CHAN(n), gain_algorithms(gain_algs), ampSweepTester(_ampSweepTester), freqSweepTester(_freqSweepTester) {};
      
    void respondToByte(char c);
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void decreaseChannelGain(int chan);

    float channelGainIncrement_dB = 2.5f;  
    int N_CHAN;
  private:
    GainAlgorithm_t *gain_algorithms;  //point to first element in array of expanders
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
};

void SerialManager::printHelp(void) {
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain settings of the device.");
  Serial.println("   C: Toggle printing of CPU and Memory usage");
  Serial.println("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  Serial.println("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  Serial.println("   A: Self-Generated Test: Amplitude sweep.");
  Serial.println("   F: Self-Generated Test: Frequency sweep.");
  Serial.print("   k: Increase the gain of all channels (ie, knob gain) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.print("   K: Decrease the gain of all channels (ie, knob gain) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.print("   1,2.3.4: Increase linear gain of given channel (1-4) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.print("   !,@,#,$: Decrease linear gain of given channel (1-4) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.println();
}

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void togglePrintMemroyAndCPU(void);
extern void togglePrintAveSignalLevels(bool);

void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB);  break;
    case '1':
      incrementChannelGain(1-1, channelGainIncrement_dB); break;
    case '2':
      incrementChannelGain(2-1, channelGainIncrement_dB); break;
    case '3':
      incrementChannelGain(3-1, channelGainIncrement_dB); break;
    case '4':
      incrementChannelGain(4-1, channelGainIncrement_dB); break;
    case '5':
      incrementChannelGain(5-1, channelGainIncrement_dB); break;
    case '6':
      incrementChannelGain(6-1, channelGainIncrement_dB); break;
    case '7':
      incrementChannelGain(7-1, channelGainIncrement_dB); break;
    case '8':      
      incrementChannelGain(8-1, channelGainIncrement_dB); break;    
    case '!':  //which is "shift 1"
      incrementChannelGain(1-1, -channelGainIncrement_dB); break;
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB); break;
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB); break;
    case '$':  //which is "shift 4"
      incrementChannelGain(4-1, -channelGainIncrement_dB); break;
    case '%':  //which is "shift 5"
      incrementChannelGain(5-1, -channelGainIncrement_dB); break;
    case '^':  //which is "shift 6"
      incrementChannelGain(6-1, -channelGainIncrement_dB); break;
    case '&':  //which is "shift 7"
      incrementChannelGain(7-1, -channelGainIncrement_dB); break;
    case '*':  //which is "shift 8"
      incrementChannelGain(8-1, -channelGainIncrement_dB); break;          
    case 'A':
      //amplitude sweep test
      { //limit the scope of any variables that I create here
        ampSweepTester.setSignalFrequency_Hz(1000.f);
        float start_amp_dB = -100.0f, end_amp_dB = 0.0f, step_amp_dB = 5.0f;
        ampSweepTester.setStepPattern(start_amp_dB, end_amp_dB, step_amp_dB);
        ampSweepTester.setTargetDurPerStep_sec(1.0);
      }
      Serial.println("Command Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      Serial.println("Press 'h' for help...");
      break;
    case 'C': case 'c':
      Serial.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemroyAndCPU(); break;
    case 'F':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-50.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 16000.f, step_octave = sqrtf(2.0);
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      Serial.println("Command Received: starting test using frequency sweep...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      Serial.println("Press 'h' for help...");
      break;      
    case 'l':
      Serial.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      Serial.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
  }
}

void SerialManager::incrementChannelGain(int chan, float change_dB) {
  if (chan < N_CHAN) {
    gain_algorithms[chan].incrementGain_dB(change_dB);
    //Serial.print("Incrementing gain on channel ");Serial.print(chan);
    //Serial.print(" by "); Serial.print(change_dB); Serial.println(" dB");
    printGainSettings();  //in main sketch file
  }
}

#endif
