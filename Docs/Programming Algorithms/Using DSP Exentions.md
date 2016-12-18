Using DSP Exentions
====================

For embedded audio processing, most of the best boards use a processor from the ARM Cortex M family.  The Teensy 3.2 and 3.6, for example, use an ARM Cortex M4.  The M4 has special features built into the chip that accelerate certain kinds of math operations that are common in Digital Signal Processing (DSP).  While these M4 processors are fast relative to a basic Arduino UNO, they're not nearly as fast as a full-blown PC.  So, if you want to get the most audio processing done on these little processors, you really need to use these DSP extensions and not rely just pure-C/C++ implementations of your favorite math operations.

CMSIS Standard Interface 
-------------------

Because ARM wants their chip cores to be widely used (which they are), and to ease a programmer's job of transitioning between different models of ARM chips (which they've successfully done), ARM has created a large collection of standardized resources that the programmer can invoke.  This is the "Cortex Microcontroller Software Interface Standard" ([CMSIS](https://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php)).  The CMSIS standard covers a wide range of resources, from real-time operating system through DSP functions.  The idea is that the programmer learns how to use them for one specific ARM Cortex chip, but that the same knowledge will also work on variety of ARM Cortex chips that the programmer might encounter in the future.  It's like the standard C library, yet optimized for the specific capabilities of the ARM Cortex family.

CMSIS DSP Exentions
----------------------

Specific to DSP operations, there is a great collection of functions in the CMSIS DSP library.  The official documentation showing all of the callable functions is [here](http://www.keil.com/pack/doc/CMSIS/DSP/html/modules.html).  I am going to show some examples below of the DSP calls that I usually need to make.  I am going to show only the floating-point versions of these functions as I am using the Teensy 3.6, which uses an ARM Cortex M4F processor, which has a floating point unit that makes these floating point calculations very fast.

### Fixed Gain

If you want to apply a fixed amount of gain to a signal, it means that you want to multiply your audio data by some gain factor.  Usually, your audio data is buffered into a block of, say, 128 points.  To make it louder or quieter, you multiply that buffer by your gain factor.  This multiplication is best done using the [Vector Scale](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__scale.html) function in the CMSIS library.  Using my floating-point extension to the Teensy library, it would look something like:

```
audio_block_t *audio = AudioStream_F32::receiveWritable_f32(); //get the audio block
float32_t gain = 2.0;  //I want to amplify the signal by 6 dB, which is a factor of 2.0
arm_scale_f32(audio->data, gain, audio->data, audio->length);  //in, gain, out, size
```

### Variable Gain

