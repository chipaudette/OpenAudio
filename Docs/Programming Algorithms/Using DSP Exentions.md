Using DSP Exentions
====================

For embedded audio processing, most of the best boards use a processor from the ARM Cortex M family.  The Teensy 3.2 and 3.6, for example, use an ARM Cortex M4.  The M4 has special features built into the chip that accelerate certain kinds of math operations that are common in Digital Signal Processing (DSP).  While these M4 processors are fast relative to a basic Arduino UNO, they're not nearly as fast as a full-blown PC.  So, if you want to get the most audio processing done on these little processors, you really need to use these DSP extensions and not rely just pure-C/C++ implementations of your favorite math operations.  

CMSIS Standard Interface 
-------------------

Because ARM wants their chip cores to be widely used (which they are), and to ease a programmer's job of transitioning between different models of ARM chips (which they've successfully done), ARM has created a large collection of standardized resources that the programmer can invoke.  This is the "Cortex Microcontroller Software Interface Standard" ([CMSIS](https://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php)).  The CMSIS standard covers a wide range of resources, from real-time operating system through DSP functions.  The idea is that the programmer learns how to use them for one specific ARM Cortex chip, but that the same knowledge will also work on variety of ARM Cortex chips that the programmer might encounter in the future.  It's like the standard C library, yet optimized for the specific capabilities of the ARM Cortex family.

Specific to DSP operations, there is a great collection of functions in the CMSIS DSP library.  The official documentation showing all of the callable functions is [here](http://www.keil.com/pack/doc/CMSIS/DSP/html/modules.html).  I am going to show some examples below of the DSP calls that I usually need to make.  I am going to show only the floating-point versions of these functions as I am using the Teensy 3.6, which uses an ARM Cortex M4F processor, which has a floating point unit that makes these floating point calculations very fast.  To use any of these DSP functions, you'll need to include the ARM math library:

``` C++
//Include ARM DSP extensions. https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <arm_math.h> 
```

Example Uses of Optimized Routines
----------------------------------

Below are some examples of how to do common audio-processing math operations in a fast way on ARM Cortex M4F processors, such as on the Teensy.

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

### Signal Power (ie, Squaring)

To compute the signal power, you can simply square the signal by multiplying each sample by itself.  If you want to square every sample in the audio buffer, though, a faster method (I think) is to use the  [Vector Multiply](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__BasicMult.html) function as we did above.

``` C++
audio_block_t *audio = AudioStream_F32::receiveWritable_f32(); //get the audio block
audio_block_f32_t *audio_pow = AudioStream_F32::allocate_f32(); //allocate space for the signal power
arm_mult_f32(audio->data, audio->data, audio_pow->data, audio->length); //in1, in2, out, length
```

### Square Root

To compute the square root of signal power (such as to compute the gain factor from the gain_power), there is no block-wise accelerated version of the square-root function.  So, you need to step through each data sample yourself.  You do, however, want to make sure that you explicitly call the floating point version of the square-root function (`sqrtf`).  It turns out that `sqrtf` is much faster than `sqrt` on ARM Cortex M4F chips (such as the Teensy).

``` C++
audio_block_f32_t *gain = AudioStream_F32::allocate_f32(); //allocate space for the signal power
float32_t gain_pow = 1.0;
float32_t update_fac = 0.001;  %what is my smoothing factor
//compute gain based on audio_pow has been computed per the example above.
for (int i=0; i < audio_pow.length; i++) {
  //let's compute a smoothed-gain that is a dynamic range compressor driving toward audio_pow = 1.0
  gain_pow = (1.0-update_fac)*gain_pow + update_fac * (1.0/(audio_pow->data[i]));
  
  //do square-root to compute the gain, not the gain_pow
  gain->data[i] = sqrtf(gain_pow);  //sqrtf is faster than sqrt!
}
arm_mult_f32(audio->data, gain->data, audio->data, audio->length); //apply the gain
```

### Logarithm and Expontial Operations

While there is no specific DSP acceleration for logarithm and exponential operations, you'll want to be sure to invoke the floating-point versions of these functions (`logf`, `log10f`, `expf`, and `powf`) because they are much faster than the generic versions.

```
//assumes that you've computed the audio_pow per the earlier example
float32_t = foo;
for (int i=0; i < audio->length; i++) {  //loop over each sample
  foo = audio_pow->data[i];  //get the value of the signal.^2
  foo = logf(foo);  //take the log base e
  foo = expf(foo);  //take the exponential (base e)
  
  foo = audio_pow->data[i];  //get the value of the signal.^2
  foo = log10f(foo);     //take the log base 10
  foo = powf(10.0,foo);  //take the exponential (base 10)
}
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

Then, inside the audio processing algorithm, we can call the filter itself via [arm_fir_f32](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__FIR.html#gae8fb334ea67eb6ecbd31824ddc14cd6a)

``` C++
audio = AudioStream_F32::receiveWritable_f32();
arm_fir_f32(&lp_filt_struct, audio->data, audio->data, audio->length); //state, in, out, length
```

### FFT and IFFT

Woking with FFT and IFFT routines on an embedded device can be challenging because these functions involve complex numbers, which are not often natively supported.  So, when creating variables to hold inputs and outputs to the FFT/IFFT routines, you need to understand how the FFT/IFFT library expects to handle the real and imaginary components of the data.  

For the ARM CMSIS FFT and IFFT routines, you need to pick the size of your FFT.  Let's assume N = 256.  Then, you need to allocate data buffers that are twice as long so that you can interleave the 256 real values with the 256 imaginary values.  For example, to setup the ARM floating-point FFT routine, you'd use [arm_cfft_radix4_init_f32](http://www.keil.com/pack/doc/CMSIS/DSP/html/group__ComplexFFT.html#gaf336459f684f0b17bfae539ef1b1b78a) and it would look something like this:

``` C++
#define N_FFT 256
arm_cfft_radix4_instance_f32 fft_inst;
uint8_t ifftFlag = 0; // 0 is FFT, 1 is IFFT
uint8_t doBitReverse = 1;
void initFFTroutines(void) {
  arm_cfft_radix4_init_f32(&fft_inst, N_FFT, ifftFlag, doBitReverse); //init FFT
}
```

Then, in your audio processing alogorithms, you would actually invoke the FFT using

``` C++
//assuming N=256, copy data from two 128-point audio buffers into the full buffer
float32_t buffer_complex[N_FFT*2];
audio_block_t *audio = AudioStream_F32::receiveWritable_f32(); //get the audio block
int targ_ind = 0;
for (int i=0; i < prev_audio.length; i++) { //copy the previous audio buffer
  buffer_complex[targ_ind++] = prev_audio.data[i]; //set the real part
  buffer_complex[targ_ind++] = 0.0;  //set the imaginary part
}
for (int i=0; i < audio.length; i++) { //copy the current audio buffer
  buffer_complex[targ_ind++] = prev_audio.data[i]; //set the real part
  buffer_complex[targ_ind++] = 0.0;  //set the imaginary part
}
prev_audio = audio; //hand off the pointer so that we have what we need next time

//you might elect to apply a windowing function here

//convert to frequency domain
arm_cfft_radix4_f32(fft_inst, buffer_complex); //output is in buffer_complex
```

To do IFFT instead of FFT, do exactly the same two-step process shown above to initialize and invoke the FFT functions.  Even the function names (`arm_cfft_radix4_init_f32` and `arm_cfft_radix4_f32`) are exactly the same to do the IFFT.  The way that you tell the code that you want an IFFT is to change the variable `ifftFlag` that is sent to `arm_cfft_radix4_init_f32` .  If it was set equal to zero, the code will do an FFT.  If it was set equal to one, the code will do an IFFT.  Easy!
