Using DSP Exentions
====================

For embedded audio processing, most of the best boards use a processor from the ARM Cortex M family.  The Teensy 3.2 and 3.6, for example, use an ARM Cortex M4.  The M4 has special features built into the chip that accelerate certain kinds of math operations that are common in Digital Signal Processing (DSP).  While these M4 processors are fast relative to a basic Arduino UNO, they're not nearly as fast as a full-blown PC.  So, if you want to get the most audio processing done on these little processors, you really need to use these DSP extensions and not rely just pure-C/C++ implementations of your favorite math operations.  

CMSIS Standard Interface 
-------------------

Because ARM wants their chip cores to be widely used (which they are), and to ease a programmer's job of transitioning between different models of ARM chips (which they've successfully done), ARM has created a large collection of standardized resources that the programmer can invoke.  This is the "Cortex Microcontroller Software Interface Standard" ([CMSIS](https://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php)).  The CMSIS standard covers a wide range of resources, from real-time operating system through DSP functions.  The idea is that the programmer learns how to use them for one specific ARM Cortex chip, but that the same knowledge will also work on variety of ARM Cortex chips that the programmer might encounter in the future.  It's like the standard C library, yet optimized for the specific capabilities of the ARM Cortex family.

CMSIS DSP Exentions
----------------------

Specific to DSP operations, there is a great collection of functions in the CMSIS DSP library.  The official documentation showing all of the callable functions is [here](http://www.keil.com/pack/doc/CMSIS/DSP/html/modules.html).  I am going to show some examples below of the DSP calls that I usually need to make.  I am going to show only the floating-point versions of these functions as I am using the Teensy 3.6, which uses an ARM Cortex M4F processor, which has a floating point unit that makes these floating point calculations very fast.  To use any of these DSP functions, you'll need to include the ARM math library:

``` C++
//Include ARM DSP extensions. https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <arm_math.h> 
```

### Fixed Gain

If you want to apply a fixed amount of gain to a signal, it means that you want to multiply your audio data by some gain factor.  Usually, your audio data is buffered into a block of, say, 128 points.  To make it louder or quieter, you multiply that buffer by your gain factor.  This multiplication is best done using the [Vector Scale](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__scale.html) function in the CMSIS library.  Using my floating-point extension to the Teensy library, it would look something like:

``` C++
audio_block_t *audio = AudioStream_F32::receiveWritable_f32(); //get the audio block
float32_t gain = 2.0;  //I want to amplify the signal by 6 dB, which is a factor of 2.0
arm_scale_f32(audio->data, gain, audio->data, audio->length);  //in, gain, out, size
```

### Variable Gain

Instead of applying a fixed gain, you may wish to vary the gain value on a sample-by-sample basis, such as to smoothly increase the volume.  In that case, you are not multiplying your audio buffer by a single gain value, but you are multiplying your audio buffer by a vector of gain values.  This is vector mulitplication -- multiplying one vector of values against a second vector of values.  This is best done using the [Vector Multiply](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__BasicMult.html) function:

``` C++
audio_block_t *audio = AudioStream_F32::receiveWritable_f32(); //get the audio block
audio_block_f32_t *gain = AudioStream_F32::allocate_f32(); //allocate space for the gain
for (int i=0; i < gain->length; i++) { gain->data[i] = (float)i/(float)gain->length); } //fade in
arm_mult_f32(audio->data, gain->data, audio->data, audio->length); //in1, in2, out, length
```

### IIR Filter

Using the CMSIS DSP library, all of the signal filtering operations can be seen [here](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__groupFilters.html).  Within this collection are a few different IIR filter types. On embedded devices, IIR filters are usually implmented as a series of "biquads", each of which are a simple second-order IIR.  If your IIR is only of order two (eg, `[b,a]=butter(2,cutoff/(fs/2));`), you simply setup a single biquad and you're done.  But, if you want a higher order IIR filter (eg, an A-weighting filter), you'll need to decompose it into a series of second order filters that you will cascade one after the other.

When using filters to a continuous audio stream an embedded device, you will be processing the audio stream in blocks.  Between blocks, you will need to hold onto the filter states so that there is no discontinuity in the audio at the edges of the blocks.  So, for any of these filter functions, you first need to invoke a setup function.  Once it is setup, you'll then invoke the filter function itself.

For me, I am used to designing filters in matlab (hence the `butter` example above).  Matlab presents its filter coefficients in a form similar to a "Type I" filter, so my example below is for a [Biquad Cascade IIR Filters Using Direct Form I Structure](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html).  With this function, you apply multiple cascaded biquad filters with a single call.  But, as this document is just an introduction, I will apply a single biquad (ie, a single second order IIR filter) only.

First, we setup the filter using its initialization routine [arm_biquad_cascade_df1_init_f32](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5).  I am setting up a high-pass filter with its cutoff at 20Hz.  In other words, I'm making a filter to get rid of an DC offset in the audio data.

``` C++
arm_biquad_casd_df1_inst_f32 hp_filt_struct; //this will hold the filter states
static const uint8_t hp_nstages = 1;
float32_t hp_coeff[5 * hp_nstages]; //allocate space for the filter coefficients
float32_t hp_state[4 * hp_nstages]; //allocate space for the filter states
void initMyFilter(void) {
  //Use matlab to compute the coeff for HP at 20Hz: [b,a]=butter(2,20/(44100/2),'high'); %assumes fs_Hz = 44100
  float32_t b[] = {9.979871156751189e-01,    -1.995974231350238e+00, 9.979871156751189e-01};  //from Matlab
  float32_t a[] = { 1.000000000000000e+00,    -1.995970179642828e+00,    9.959782830576472e-01};  //from Matlab
  
  //prepare the coefficients //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html
  hp_coeff[0] = b[0];   hp_coeff[1] = b[1];  hp_coeff[2] = b[2]; //here are the matlab "b" coefficients
  hp_coeff[3] = -a[1];  hp_coeff[4] = -a[2];  //the DSP needs the "a" terms to have opposite sign vs Matlab
  
  //call the initialization function
  arm_biquad_cascade_df1_init_f32(&hp_filt_struct, hp_nstages, hp_coeff, hp_state);
}
```

Then, inside the audio processing algorithm, we can call the filter itself via [arm_biquad_cascade_df1_f32](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#gaa0dbe330d763e3c1d8030b3ef12d5bdc).

``` C++
audio = AudioStream_F32::receiveWritable_f32();
arm_biquad_cascade_df1_f32(&hp_filt_struct, audio->data, audio->data, audio->length); //state, in, out, length
```

### FIR Filter

Setting up and using an CMSIS [FIR filter](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__FIR.html) is much like setting up and using a CMSIS IIR filter, except that you don't need to limit yourself to second-order filters.  With FIR, you can use whatever length you'd like, simply by telling it your desired length during setup.  In addition to my example code below, the CMSIS docs have their own Matlab-centric example of their FIR routine [here](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__FIRLPF.html).

As with the IIR filter, you need to setup the FIR filter before you can use it.  For the FIR filter, the setup routine [arm_fir_init_f32](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__FIR.html#ga98d13def6427e29522829f945d0967db) can be used in a way like:

``` C++
arm_fir_instance_f32 lp_filt_struct; //this will hold the filter states
static const uint16_t n_taps = 16;
static const uint16_t block_size = AUDIO_BLOCK_SAMPLES;
//Use matlab to compute the coeff for HP at 4000 Hz: [b,a]=fir1(16-1,4000/(44100/2)); %assumes fs_Hz = 44100
float32_t lp_coeff[n_taps] = {-3.21485003e-03,  -3.27201675e-03,   1.00097857e-04,  1.60527544e-02, 
         5.09739046e-02,   1.01419356e-01,   1.52699318e-01,   1.85241436e-01, 
         1.85241436e-01,   1.52699318e-01,   1.01419356e-01,   5.09739046e-02, 
         1.60527544e-02,   1.00097857e-04,  -3.27201675e-03,  -3.21485003e-03 };
float32_t lp_state[n_taps+block_size-1]; //allocate space for the filter states
void initMyFilter(void) {
  //call the initialization function
  arm_fir_init_f32(&lp_filt_struct, n_taps, lp_coeff, lp_state, block_size);
}
```
