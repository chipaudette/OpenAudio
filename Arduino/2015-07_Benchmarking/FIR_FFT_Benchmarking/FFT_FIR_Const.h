
#ifndef _FFT_FIR_CONST
#define _FFT_FIR_CONST


// Define the platform that you're using
//#define IS_ARDUINO_UNO  //unless you're using an UNO, comment this out for Teensy or for NXP Freedom board
#define IS_ARDUINO_IDE  //if you're outside of the Arduino IDE, comment this out 

// Define the data type that you'd like
#define USE_INT16 0  //don't need to change this
#define USE_INT32 1  //don't need to change this
#define USE_FLOAT 2  //don't need to change this
#define DATA_TYPE  USE_FLOAT  //Change this to one of the choices above

#ifdef IS_ARDUINO_IDE
#include <arduino.h> //for micros() and whatnot
#endif

// Define the maximum size of the FFT/FIR that you want to do
#ifdef IS_ARDUINO_UNO  //Arduino Uno has very little memory
  #if (DATA_TYPE == USE_INT16)
    #if (OPERATION_TO_DO == DO_NAIVE_FIR)
        #define MAX_N 256
    #else
        #define MAX_N 64   
    #endif
  #else
    #if (OPERATION_TO_DO == DO_NAIVE_FIR)
        #define MAX_N 128
    #else
        #define MAX_N 32    //Arduino Uno has very little memory
    #endif
  #endif
#else
  #define MAX_N 1024  //All other platforms have more memory so you can do more
#endif

// ///////////// Below is code that responds to your choices above
#ifdef IS_ARDUINO_UNO
//define data types
typedef int int16_t;
typedef long int32_t;  
typedef unsigned long uint32_t;
#endif

//for NAIVE_FIR
#if (DATA_TYPE == USE_INT16)
typedef int16_t filt_t;
#elif (DATA_TYPE == USE_INT32)
typedef int32_t filt_t;
#elif (DATA_TYPE == USE_FLOAT)
typedef float filt_t;
#endif
  

//for KISS_FFT
#if (DATA_TYPE == USE_INT16)
#define FIXED_POINT 16
#elif (DATA_TYPE == USE_INT32)
#define FIXED_POINT 32
#endif


#endif
