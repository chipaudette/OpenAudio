Computing Filter Coefficients
==============================

The purpose of this doc is to show how filter coefficients can be computing using different tools.

Requirements
----------

**Matlab**: To compute filter coefficients in Matlab, you need to have both Matlab and the Signal Processing Toolbox.  If you have both of these products, you have a very powerful toolset for designing filters and visualizing their performance.

**Python**: To compute filter coefficients in Python, you can use a variety of packages, but I use SciPy.  Since filtering is usually done in conjunction with other packages such as NumPy and matplotlib, I tend to use a fully-packaged distribution such as [Anaconda](https://www.continuum.io/downloads).  For more information on how on emight move from Matlab to Python, you can check out my earlier post [here](http://eeghacker.blogspot.com/2014/10/moving-from-matlab-to-python.html).

FIR Filters
-----------

### Matlab

If you want a simple low-pass, high-pass, or bandpass FIR filter, all you need is Matlab's [fir1](https://www.mathworks.com/help/signal/ref/fir1.html) command.  It can be used like this:

``` Matlab
% setup the fiter
fs_Hz = 44100;     %sample rate
cutoff_Hz = 4000;  %cutoff frequency
N_fir = 32;        %filter order
ftype = 'low';     % choose a low-pass fiter
[b,a]=fir1(N_fir,cutoff_Hz/(fs_Hz/2),ftype);

% call the filter
f_my_data = filter(b,a,my_data);
```

### Python

In SciPy, I believe that the function most similar to Matlab's `fir1` is the function `firwin`.  It differs mainly in how you request a low-pass vs high-pass vs bandpass.  Below I show a low-pass to mirror the Matlab example above.  For other filter types, see the examples at the end of the official [doc](https://docs.scipy.org/doc/scipy-0.18.1/reference/generated/scipy.signal.firwin.html#scipy.signal.firwin)

``` Python
import numpy as np
from scipy import signal

# setup the filter
fs_Hz = 44100.0    # sample rate
cutoff_Hz = 4000   # cutoff frequency
N_taps = 32        # filter order
b, a = signal.firwin(N_taps, cutoff_Hz/(fs_Hz/2.0))

# call the filter
f_my_data = signal.lfilter(b, a, my_data, 0) # apply along the zeroeth dimension
```

IIR Filters
-----------

### Matlab

For an IIR filter, there are a variety of IIR filter types that you can use.  I mostly use butterworth filters, so I motly use the [butter](https://www.mathworks.com/help/signal/ref/butter.html) command.  As a result, my code looks like:

``` Matlab
% setup the fiter
fs_Hz = 44100;     %sample rate
cutoff_Hz = 4000;  %cutoff frequency
N_iir = 2;         %filter order
ftype = 'low';     % choose a low-pass fiter
[b,a]=butter(N_iir,cutoff_Hz/(fs_Hz/2),ftype);

% call the filter
f_my_data = filter(b,a,my_data);
```

### Python

For an IIR filter, SciPy includes a number of functions that parallel the Matlab style.  So, if you like butterworth filters, SciPy also has a [butter](https://docs.scipy.org/doc/scipy-0.18.1/reference/generated/scipy.signal.butter.html#scipy.signal.butter) command.  As a result, my code looks like:

``` Python
import numpy as np
from scipy import signal

# setup the filter
fs_Hz = 44100.0    # sample rate
cutoff_Hz = 4000   # cutoff frequency
N_iir = 2          # filter order
ftype = 'lowpass'; # pick low-pass filter
b, a = signal.butter(N_iir, cutoff_Hz/(fs_Hz/2.0),ftype)

# call the filter
f_my_data = signal.lfilter(b, a, my_data, 0) # apply along the zeroeth dimension
```

