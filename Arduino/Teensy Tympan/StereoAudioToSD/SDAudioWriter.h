
#ifndef _SDAudioWriter_h
#define _SDAudioWriter_h

#include <Audio.h>
#include <SD.h>
#include <Tympan_Library.h>

#define MAX_AUDIO_BUFF_LEN (2*AUDIO_BLOCK_SAMPLES)  //Assume stereo (for the 2x).  AUDIO_BLOCK_SAMPLES is from Audio_Stream.h, which should be max of Audio_Stream_F32 as well.
class SDAudioWriter {
  public:
    SDAudioWriter() {};
    ~SDAudioWriter() {
      if (isFileOpen()) { close(); }
    }

    int setup(int _chipSelect) {
      chipSelect = _chipSelect;
      return setup();
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

    void enablePrintElapsedWriteTime(void) {
      flagPrintElapsedWriteTime = true;
    }
    void disablePrintElapseWriteTime(void) {
      flagPrintElapsedWriteTime = false;
    }

    //write one Int16 channels as int16
    int writeInt16AsInt16(int16_t *chan1,  int nsamps) {
      if (frec) {
        // write all audio bytes (note that 512 bytes is most efficient)
        elapsedMicros usec = 0;
        frec.write((byte *)chan1, nsamps*sizeof(chan1[0]));
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return 0;
    }

    //write two Int16 channels as int16
    int writeInt16AsInt16(int16_t *chan1, int16_t *chan2, int nsamps) {
      const int buffer_len = nsamps*2;  //it'll be stereo, so 2*nsamps
      int count=0;
      if (frec) {
        for (int Isamp=0; Isamp < nsamps; Isamp++) {
          //interleave
          write_buffer[count++] = chan1[Isamp];
          write_buffer[count++] = chan2[Isamp];
        }
     
        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime)  usec = 0;
        frec.write((byte *)write_buffer, buffer_len*sizeof(write_buffer[0]));
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return 0;
    }

    //write one F32 channel as int16
    int writeF32AsInt16(float32_t *chan1, int nsamps) {
      const int buffer_len = nsamps;  
      int count=0;
      if (frec) {
        for (int Isamp=0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16
          write_buffer[count++] = (int16_t)(chan1[Isamp]*32767.0);
          //write_buffer[count++] = (int16_t)(chan2[Isamp]*32767.0);
        }
     
        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime)  usec = 0;
        frec.write((byte *)write_buffer, buffer_len*sizeof(write_buffer[0]));
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return 0;
    }

    //write two F32 channels as int16
    int writeF32AsInt16(float32_t *chan1, float32_t *chan2, int nsamps) {
      const int buffer_len = 2*nsamps;  //it'll be stereo, so 2*nsamps
      int count=0;
      if (frec) {
        for (int Isamp=0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16 and interleave
          write_buffer[count++] = (int16_t)(chan1[Isamp]*32767.0);
          write_buffer[count++] = (int16_t)(chan2[Isamp]*32767.0);
        }
     
        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime)  usec = 0;
        frec.write((byte *)write_buffer, buffer_len*sizeof(write_buffer[0]));
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return 0;
    }

    //write two channels as float32
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
        if (flagPrintElapsedWriteTime)  usec = 0;
        frec.write((byte *)buffer, nsamps*sizeof(buffer[0]));
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return 0;
    }

    int close(void) {
      frec.close();
      return 0;
    }

  private:
    int chipSelect = BUILTIN_SDCARD;  //for Teensy 3.6
    File frec;  //Create the file object for the SD card
    int16_t write_buffer[MAX_AUDIO_BUFF_LEN];
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    //int SD_mode = 0;   // 0 is not recording, 1 is recording    
  
};

#endif
