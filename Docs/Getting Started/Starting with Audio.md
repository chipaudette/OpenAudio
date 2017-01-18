Getting Started with Audio
===========================

Assuming that you are familiar with Arduino and Teensy (if not, see [this page](https://github.com/chipaudette/OpenAudio/blob/master/Docs/Getting%20Started/Starting%20From%20Scratch.md)), you're ready to processing audio.  This document will get you started with the Teensy Audio Library and with using my OpenAudio extensions.

Teensy Audio Library
------------

My approach to audio processing with the Teensy relies upon the existing foundation of Teensy's own Audio Library.  This library should have been installed on your computer when you installed the Teensyduino stuff when you were getting started with the Teensy.  So, you should be good-to-go.

If you're particularly interested, you can see the Audio Library documenation [here](http://www.pjrc.com/teensy/td_libs_Audio.html).  When those docs are insufficient, you can usually find what you need by digging into its code at its [GitHub](https://github.com/PaulStoffregen/Audio).  

If you need to get even deeper, some of the fundamental constants and classes for the Audio Library are not actually in the Audio Library.  They're buried deep in the repository for the Teensy Core itself (see [AudioStream.h](https://github.com/PaulStoffregen/cores/blob/master/teensy3/AudioStream.h) and [AudioStream.cpp](https://github.com/PaulStoffregen/cores/blob/master/teensy3/AudioStream.cpp)).  

Again, if the Teensy works with your computer, you already have these libraries and you don't need to do any more.

First Audio Trials
-----------------

If you have a Teensy and a Teensy Audio Adapter (which is what my breadboard prototype uses), you'll want to start with some very simple audio trials.  These are aimed at making your hardware do something (anything!) and to let you see the basic structure of what a Teensy Audio Processing program looks like:

* **Audio Pass-Thru:**  You should start with this blog post: [Teensy Audio Board - First Audio](http://openaudio.blogspot.com/2016/10/teensy-audio-board-first-audio.html). It walks you through the process of configuring your device to pass the input audio to the output.  No processing, just an audio pass-thru.  Always start simple.  It doesn't get simpler than this.  If you don't want to walk through the whole process of using the Teensy Audio GUI, you can use my code directly.  Get it at the OpenAudio GitHub [here](https://github.com/chipaudette/OpenAudio_blog/tree/master/2016-10-23%20First%20Teensy%20Audio/Arduino/BasicLineInPassThrough).

* **Basic Gain:** As your second trial, you should try adding gain to the signal.  Go to this blog post: [A Teensy Hearing Aid](http://openaudio.blogspot.com/2016/11/a-teensy-hearing-aid.html).  In addition to introducing the hardware of the hearing aid breadboad, it points to code that adds gain to the audio signal based on the setting of the potentiometer.  It helps illustrate how the data flows through the system so that you can manipulate it.  If you don't want to read through the whole thing, the example code can be found directly at my OpenAudio GitHub [here](https://github.com/chipaudette/OpenAudio_blog/tree/master/2016-11-20%20Basic%20Hearing%20Aid/BasicGain).

Floating-Point Audio Processing Library
-------------------------------

After these first two demos, you know that your hardware works and you can start thinking about implementing your own audio algorithms.  The two examples above illustrate audio processing using fixed-point data types (ie, Int16).  I find it much easier to develop my algorithms using floating-point operations, like used by Matlab.  Therefore, I have been extending pieces of the Teensy Audio Library to enable floating-point math.  You will need to download my extended library.  You have two options:

* **Option 1: Simpler**:  The simpler way to get this library is to download a ZIP'd version of the library from my the github repo for this library.  Go to the GitHub page for the repo [here](https://github.com/chipaudette/OpenAudio_ArduinoLibrary).  On the right side, click on the green button that says "Clone or Download" and select "Download Zip".  Once it is on your computer, unzip it and drag the "OpenAudio_ArduinoLibrary" folder (renaming it as necessary) into your Arduino libraries directory.  On my Windows PC, Arduino libraries go under "My Documents\Arduino\Libraries\".

* **Option 2: Better Long-Term**:  Longer term, you'll want to be able to update this library automatically as I (and the community) continue to update it.  You could update it by downloading a new ZIP each time, but the better method is to have it be managed via Git.  If you have a GitHub account, you should clone (or fork) my repository.  To pull it down to your computer, clone it directly to your Arduino libraries directory ("My Documents\Arduino\Libraries\") so that the Arduino IDE can find it.

Floating-Point Examples
-------------------------

Once you have the library, you should try the example programs that come with the library.  If you've installed the library correctly in the Arduino libraries directory, the example will show up in the Arduino IDE "examples" menu.  Go under the "File" menu and select "Examples".  Scroll way down and look for "OpenAudio_ArduinoLibrary".  Select "BasicGain_Float".  Compile it and make sure it works.  You can see some explanation of how it works on this blog post: [Extending Teensy Audio Library for Floating-Point](http://openaudio.blogspot.com/2016/12/extending-teensy-audio-library-for.html).

![Exampes](https://4.bp.blogspot.com/-m5f4ZGKg5pw/WEM6ARN-BAI/AAAAAAAAEEc/IAby6zgbSTcAa2Dr4hmqlBYV2bh6eD13QCEw/s400/Screenshot_examples.png "Examples")

After this example of basic gain, you can see an example of dynamic compression here (TBD!) and of frequency-domain processing here (TDB!)

Making Your Own Algorithms
---------------------------

To make your own algorithms, I would start with the blank template here: [MyAudioEffect_Float](https://github.com/chipaudette/OpenAudio_ArduinoLibrary/tree/master/examples/MyAudioEffect_Float).  This example has all the pieces of code that are required to work with the Teensy Audio Library to get the audio samples and prepare them for your algorithm.  You can add your algorithm in the spot indicated in the file `AudioEffectMine_F32.h` that is included with the example.

When coding up your audio processingalgorithm, it is important to remember that your embedded processor (the Teensy!) is not as fast as your PC.  The embedded processor is much lower.  So, when coding, you must ensure that you do your audio calculations in an efficient manner.  For typical audio-related math operations (multiplication, square root, logarithms, IIR and FIR filtering, and FFT conversion), you'll want to use the guidance that I've provided [here](https://github.com/chipaudette/OpenAudio/blob/master/Docs/Programming%20Algorithms/Using%20DSP%20Exentions.md).  Using the fastest routines makes a HUGE difference in how much processing you can do.

