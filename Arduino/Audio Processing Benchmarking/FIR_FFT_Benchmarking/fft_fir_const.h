
#ifndef _FFT_FIR_CONST
#define _FFT_FIR_CONST


// Define the platform that you're using
//#define IS_ARDUINO_UNO  //Enable this only if using an UNO  Comment this out for Teensy or Maple or for NXP Freedom board
#define IS_ARDUINO_IDE  //Enable this for Arduino or Teensy.  If not using Arduino ID (ie, NXP Freedom board) comment this out.
//#define IS_MAPLE   //Enable this for Maple via Arduino IDE from: https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki/Installation

// Define the data type that you'd like
#define USE_INT16 0  //you don't need to change this
#define USE_INT32 1  //you don't need to change this
#define USE_FLOAT 2  //you don't need to change this
#define DATA_TYPE  USE_INT16  //THIS is the one to change...copy-paste one of the three types above


#ifdef IS_ARDUINO_IDE
  #ifdef IS_MAPLE
    #include <stdlib.h> //does some of the datatypes for Maple
  #else
    #include <arduino.h> //for micros() and whatnot for arduino.  Don't do this for Maples
  #endif
#else
  #include "board.h" //for Kinetis IDE
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
  #ifdef IS_MAPLE
    #if (OPERATION_TO_DO == DO_NAIVE_FIR)
        #define MAX_N 1024
    #else
        #define MAX_N 256   //limit for KissFFT
    #endif
  #else
    //Arduino M0, Due (?), Teensy
    #define MAX_N 1024  //All other platforms have more memory so you can do more
  #endif
#endif

// ///////////// Below is code that responds to your choices above
#ifdef IS_ARDUINO_UNO
//define data types
typedef int int16_t;
typedef long int32_t;  
typedef unsigned long uint32_t;
#endif

#ifdef IS_MAPLE
//define data types...http://docs.leaflabs.com/static.leaflabs.com/pub/leaflabs/maple-docs/0.0.11/lang/cpp/built-in-types.html
typedef short int int16_t;
//typedef int int32_t;  
//typedef int __int32_t;
typedef __INT32_TYPE__ __int32_t;
typedef __int32_t int32_t;  
typedef unsigned long uint32_t;
typedef long long int64_t;
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
