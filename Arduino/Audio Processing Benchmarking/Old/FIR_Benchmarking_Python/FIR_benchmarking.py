
import matplotlib.pyplot as plt
import numpy as np
import timeit
import time
import scipy.signal as signal

def fir_filter(b,in_val,z,z_ind,N_FIR):
    z_ind = z_ind+1
    if (z_ind == N_FIR):
        z_ind = 0 # increment and wrap (if necessary)
    z[z_ind] = in_val; # put new value into z
      
    #apply filter
    out_val = 0.0
    foo_z_ind = z_ind  #not needed for modulo
    for i in range(N_FIR):
      foo_z_ind -= 1 
      if (foo_z_ind < 0):
          foo_z_ind = N_FIR-1
      
      out_val += b[i]*z[foo_z_ind];
    
    return out_val, z, z_ind

def fir_filter_vect(b,a,in_val,z):
    out_val, z = signal.lfilter(b,a,in_val,zi=z)
    return out_val, z


def test_FIR_N(N_LOOP, N_FIR, b, a, in_val,z):
    #z_ind = 0
    for Iloop in range(N_LOOP):
        #out_val, z, z_ind = fir_filter(b,in_val,z,z_ind, N_FIR)
        #out_val,z = fir_filter_vect(b,a,in_val,z)
        out_val, z = signal.lfilter(b,a,in_val,zi=z)
        
    return out_val,z





N_LOOP = 100000


for N_FIR in [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384]:
#for N_FIR in [1024, 2048, 4096, 8192, 16384]:
    #initialize filters
    #N_FIR = 128
    #z = np.zeros(N_FIR)
    #b = 12.345*np.ones(N_FIR)
    #z_ind = 0
    in_val = 134.0*np.ones(1)
    
    #out_val = test_FIR_N(N_LOOP, N_FIR, b, z)
    #print out_val
    
    #get the correct size of z
    b = signal.firwin(N_FIR,0.1)
    a = np.ones(1)
    z = np.zeros(len(b)-1)
    #outval, z = signal.lfilter(b,a,np.ones([2]),zi=z)    
    #print z
    
    time1 = time.time()
    out_val,z = test_FIR_N(N_LOOP, N_FIR, b, a, in_val, z)
    time2 = time.time()
    print 'N=%i took %0.3f usec per call ' % (N_FIR, (time2-time1)*1.0e6/float(N_LOOP))
    #print out_val




    
    