Getting Started with Audio
===========================

Assuming that you are familiar with Arduino and Teensy (if not, see this page), you're ready to get started with the audio processing using the Teensy Audio Library along with my OpenAudio extensions.

Teensy Audio Library
------------

To do the audio processing examples, you'll need the Teensy Audio Library, which should have been installed on your computer when you installed the Teensyduino stuff when you were getting started with the Teensy.  If you're interested, you can see the Audio Library documenation [here](http://www.pjrc.com/teensy/td_libs_Audio.html).  When those docs are insufficient, you can usually find what you need by digging into its code at its [GitHub](https://github.com/PaulStoffregen/Audio).  And, if you need to see some of the fundamental constants and classes for the Audio Library, they're actually buried in the repository for the Teensy Core (see [AudioStream.h](https://github.com/PaulStoffregen/cores/blob/master/teensy3/AudioStream.h) and [AudioStream.cpp](https://github.com/PaulStoffregen/cores/blob/master/teensy3/AudioStream.cpp)).  Again, if the Teensy works with your computer, you already have these libraries and you don't need to do any more.

Getting Your First Audio
-----------------

If you have a Teensy and a Teensy Audio Adapter (my breadboard prototypes, for example, use this Audio Adapter), you should start with this blog post: [A Teensy Hearing Aid](http://openaudio.blogspot.com/2016/11/a-teensy-hearing-aid.html).  It walks you through the process of configuring your device to pass the input audio to the output.  No processing, just an audio pass-thru.  Always start simple.  It doesn't get simpler than this.

If you don't want to walk through the whole process of using the Teensy Audio GUI, you can use my code directly.  Get it at the OpenAudio github [here](https://github.com/chipaudette/OpenAudio_blog/tree/master/2016-10-23%20First%20Teensy%20Audio/Arduino/BasicLineInPassThrough).



**OpenAudio Library**: The Teensy Audio Library assumes that all of the audio data (and most of the audio processing operations) are Int16 operations.  I find that floating-point data is much easier to work with, so my 
