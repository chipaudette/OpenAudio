
#ifndef _SerialManager_h
#define _SerialManager_h

//add in the algorithm whose gains we wish to set via this SerialManager
#include "AudioEffectExpander.h"
typedef AudioEffectExpander_F32 GainAlgorithm_t;

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(int n, GainAlgorithm_t *gain_algs) { 
      N_CHAN = n; 
      gain_algorithms = gain_algs;
    };
    void respondToByte(char c);
    
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void decreaseChannelGain(int chan);

    float channelGainIncrement_dB = 2.5f;  
    int N_CHAN;
  private:
    GainAlgorithm_t *gain_algorithms;  //point to first element in array of expanders
  
};

void SerialManager::printHelp(void) {
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain settings of the device.");
  Serial.println("   C: Toggle printing of CPU and Memory usage");
  Serial.println("   L: Toggle printing of pre-gain per-channel signal levels");
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
extern void togglePrintAveSignalLevels(void);

//switch yard to determine the desired action
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
    case '!':  //which is "shift 1"
      incrementChannelGain(1-1, -channelGainIncrement_dB); break;
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB); break;
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB); break;
    case '$':  //which is "shift 4"
      incrementChannelGain(4-1, -channelGainIncrement_dB); break;          
    case 'C': case 'c':
      Serial.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemroyAndCPU(); break;
    case 'L': case 'l':
      Serial.println("Command Received: toggle printing of per-band ave signal levels.");
      togglePrintAveSignalLevels(); break;
  }
}

void SerialManager::incrementChannelGain(int chan, float change_dB) {
  if (chan < N_CHAN) {
    gain_algorithms[chan].incrementGain_dB(change_dB);
    Serial.print("Incrementing gain on channel ");Serial.print(chan);
    Serial.print(" by "); Serial.print(change_dB); Serial.println(" dB");
    printGainSettings();  //in main sketch file
  }
}

#endif

