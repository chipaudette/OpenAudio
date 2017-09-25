
#ifndef _MyWriteAudioToSD_h
#define _MyWriteAudioToSD_h

#include <Audio.h>
//include <Wire.h>
//include <SPI.h>
#include <SD.h>
//include <SerialFlash.h>
#include <Tympan_Library.h>

class MyWriteAudioToSD {
  public:
    MyWriteAudioToSD() {};
    ~MyWriteAudioToSD() {
      if (isFileOpen()) { close(); }
    }
    
    int setup(void) {
      if (!(SD.begin(chipSelect))) {
        // stop here if no SD card, but print a message
        while (1) {
          Serial.println("MyWriteAudioToSD: setup: Unable to access the SD card...");
          delay(500);
        }
      }
      return 0;
    }

    bool open(char *fname) {
      if (SD.exists(fname)) {
        // The SD library writes new data to the end of the
        // file, so to start a new recording, the old file
        // must be deleted before new data is written.
        SD.remove(fname);
      }
      frec = SD.open(fname, FILE_WRITE);
      return isFileOpen();
    }

    bool isFileOpen(void) {
      if (frec) {
        return true;
      } else {
        return false;
      }
    }

    //write two channels as int16
    int writeF32AsInt16(float32_t *chan1, float32_t *chan2, int nsamps) {
      const int buffer_len = 2*nsamps;  //it'll be stereo, so 2*nsamps
      int16_t buffer[buffer_len];
      int count=0;
      if (frec) {
        for (int Isamp=0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16 and interleave
          buffer[count++] = (int16_t)(chan1[Isamp]*32767.0);
          buffer[count++] = (int16_t)(chan2[Isamp]*32767.0);
        }
     
        // write all audio bytes (512 bytes is most efficient)
        frec.write((byte *)buffer, buffer_len*sizeof(buffer[0]));
      }
      return 0;
    }

    //write two channels as int16
    int writeF32AsF32(float32_t *chan1, float32_t *chan2, int nsamps) {
      float32_t buffer[nsamps*2];
      int count=0;
      if (frec) {
        for (int Isamp=0; Isamp < nsamps; Isamp++) {
          //interleave
          buffer[count++] = chan1[Isamp];
          buffer[count++] = chan2[Isamp];
        }
     
        // write all audio bytes (512 bytes is most efficient)
        frec.write((byte *)buffer, nsamps*sizeof(buffer[0]));
      }
      return 0;
    }

    int close(void) {
      frec.close();
      return 0;
    }

  private:
    const int chipSelect = BUILTIN_SDCARD;  //for Teensy 3.6
    File frec;  //Create the file object for the SD card
    //int SD_mode = 0;   // 0 is not recording, 1 is recording    
  
};

#endif
