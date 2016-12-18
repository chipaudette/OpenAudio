Computing Filter Coefficients
==============================

The purpose of this doc is to show how filter coefficients can be computing using different tools.

Requirements
----------

**Matlab**: To compute filter coefficients in Matlab, you need to have both Matlab and the Signal Processing Toolbox.  If you have both of these products, you have a very powerful toolset for designing filters and visualizing their performance.

**Python**: To compute filter coefficients in Python, you can use a variety of packages, but I use scipy.  Since filtering is usually done in conjunction with other packages such as numpy and matplotlib, I tend to use a fully-packaged distribution such as [Anaconda](https://www.continuum.io/downloads).  For more information on how on emight move from Matlab to Python, you can check out my earlier post [here](http://eeghacker.blogspot.com/2014/10/moving-from-matlab-to-python.html).

FIR Filters
-----------

### Matlab

If you want a simple low-pass, high-pass, or bandpass FIR filter, all you need is Matlab's [fir1](https://www.mathworks.com/help/signal/ref/fir1.html) command.  It can be used like this:

``` Matlab
fs_Hz = 44100;  %sample rate
cutoff_Hz = 4000;  %cutoff frequency
N_fir = 32;  %filter order
ftype = 'low';  % choose a low-pass fiter
[b,a]=fir1(N_fir,cutoff_Hz/(fs_Hz/2),ftype);
```

### IIR Filter
