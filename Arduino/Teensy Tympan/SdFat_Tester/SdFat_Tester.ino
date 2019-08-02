

//#include <SdFs.h>  //https://github.com/greiman/SdFs
#include <SdFat.h>  //https://github.com/greiman/SdFat


//SDWriter:  This is a class to write blocks of bytes, chars, ints or floats to
//  the SD card.  It will write blocks of data of whatever the size, even if it is not
//  most efficient for the SD card.  This is a base class upon which other classes
//  can inheret.  The derived classes can then do things more smartly, like write
//  more efficient blocks of 512 bytes.
//
//  To handle the interleaving of multiple channels and to handle conversion to the
//  desired write type (float32 -> int16) and to handle buffering so that the optimal
//  number of bytes are written at once, use one of the derived classes such as
//  BufferedSDWriter
class SDWriter : public Print
{
  public:
    SDWriter() {};
    SDWriter(Print* _serial_ptr) {
      setSerial(_serial_ptr);
    };
    virtual ~SDWriter() {
      if (isFileOpen()) close();
    }

    void setup(void) { init(); }
    virtual void init() {
      if (!sd.begin()) sd.errorHalt(serial_ptr, "SDWriter: begin failed");
    }

    bool openAsWAV(char *fname) {
      bool returnVal = open(fname);
      if (isFileOpen()) { //true if file is open
        flag__fileIsWAV = true;
        file.write(wavHeaderInt16(0), WAVheader_bytes); //initialize assuming zero length
      }
      return returnVal;
    }

    bool open(char *fname) {
      if (sd.exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
        // The SD library writes new data to the end of the file, so to start
        //a new recording, the old file must be deleted before new data is written.
        sd.remove(fname);
      }
      file.open(fname, O_RDWR | O_CREAT | O_TRUNC);
      //file.createContiguous(fname, PRE_ALLOCATE_SIZE); //alternative to the line above
      return isFileOpen();
    }


    int close(void) {
      //file.truncate(); 
      if (flag__fileIsWAV) {
        //re-write the header with the correct file size
        uint32_t fileSize = file.fileSize();//SdFat_Gre_FatLib version of size();
        file.seekSet(0); //SdFat_Gre_FatLib version of seek();
        file.write(wavHeaderInt16(fileSize), WAVheader_bytes); //write header with correct length
        file.seekSet(fileSize);
      }
      file.close();
      flag__fileIsWAV = false;
      return 0;
    }

    bool isFileOpen(void) {
      if (file.isOpen()) return true;
      return false;
    }

    //This "write" is for compatibility with the Print interface.  Writing one
    //byte at a time is EXTREMELY inefficient and shouldn't be done
    virtual size_t write(uint8_t foo)  {
      size_t return_val = 0;
      if (file.isOpen()) {

        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime) { usec = 0; }
        file.write((byte *) (&foo), 1); //write one value
        return_val = 1;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return return_val;
    }

    //write Byte buffer...the lowest-level call upon which the others are built.
    //writing 512 is most efficient (ie 256 int16 or 128 float32
    virtual size_t write(const uint8_t *buff, int nbytes) {
      size_t return_val = 0;
      if (file.isOpen()) {
        if (flagPrintElapsedWriteTime) { usec = 0; }
        file.write((byte *)buff, nbytes); return_val = nbytes;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return return_val;
    }
    virtual size_t write(const char *buff, int nchar) { 
      return write((uint8_t *)buff,nchar); 
    }

    //write Int16 buffer.
    virtual size_t write(int16_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    //write float32 buffer
    virtual size_t write(float *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    void setPrintElapsedWriteTime(bool flag) { flagPrintElapsedWriteTime = flag; }
    
    virtual void setSerial(Print *ptr) {  serial_ptr = ptr; }
    virtual Print* getSerial(void) { return serial_ptr;  }

    int setNChanWAV(int nchan) { return WAV_nchan = nchan;  };
    float setSampleRateWAV(float sampleRate_Hz) { return WAV_sampleRate_Hz = sampleRate_Hz; }

    //modified from Walter at https://github.com/WMXZ-EU/microSoundRecorder/blob/master/audio_logger_if.h
    char * wavHeaderInt16(const uint32_t fsize) {
      return wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, fsize);
    }
    char* wavHeaderInt16(const float sampleRate_Hz, const int nchan, const uint32_t fileSize) {
      //const int fileSize = bytesWritten+44;

      int fsamp = (int) sampleRate_Hz;
      int nbits = 16;
      int nbytes = nbits / 8;
      int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);

      static char wheader[48]; // 44 for wav

      strcpy(wheader, "RIFF");
      strcpy(wheader + 8, "WAVE");
      strcpy(wheader + 12, "fmt ");
      strcpy(wheader + 36, "data");
      *(int32_t*)(wheader + 16) = 16; // chunk_size
      *(int16_t*)(wheader + 20) = 1; // PCM
      *(int16_t*)(wheader + 22) = nchan; // numChannels
      *(int32_t*)(wheader + 24) = fsamp; // sample rate
      *(int32_t*)(wheader + 28) = fsamp * nbytes; // byte rate
      *(int16_t*)(wheader + 32) = nchan * nbytes; // block align
      *(int16_t*)(wheader + 34) = nbits; // bits per sample
      *(int32_t*)(wheader + 40) = nsamp * nchan * nbytes;
      *(int32_t*)(wheader + 4) = 36 + nsamp * nchan * nbytes;

      return wheader;
    }
    
  protected:
    //SdFatSdio sd; //slower
    SdFatSdioEX sd; //faster
    SdFile file;
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    Print* serial_ptr = &Serial;
    bool flag__fileIsWAV = false;
    const int WAVheader_bytes = 44;
    float WAV_sampleRate_Hz = 44100.0;
    int WAV_nchan = 2;
};

byte output_buff[512];
int loop_count = 0;
bool isFileOpen = false;
SDWriter mySDwriter;

void setup() {
  Serial.begin(115200); delay(1000);
  Serial.println("SdFs Tester...");
  delay(1000);

  Serial.println("Opening SD file: test.wav");
  isFileOpen = mySDwriter.openAsWAV("test.wav");
  if (isFileOpen) {
    Serial.println("File opened successfully.");
  } else {
    Serial.println("Failed to open file.");
  }
  
  
}

void loop() {

  if (loop_count < 250) {
    
    if (isFileOpen) {   
      loop_count++;
      for (int i=0; i<512; i++) output_buff[i]=loop_count;
      mySDwriter.write(output_buff,512);
    }
    
  } else {
    
    if (isFileOpen) {
      //close file
      isFileOpen = false;
    }
    delay(10);
  }
  
}
