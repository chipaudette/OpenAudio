/*      fix_fft.c - Fixed-point Fast Fourier Transform  */
/*
        fix_fft()       perform FFT or inverse FFT
        window()        applies a Hanning window to the (time) input
        fix_loud()      calculates the loudness of the signal, for
                        each freq point. Result is an integer array,
                        units are dB (values will be negative).
        iscale()        scale an integer value by (numer/denom).
        fix_mpy()       perform fixed-point multiplication.
        Sinewave[1024]  sinewave normalized to 32767 (= 1.0).
        Loudampl[100]   Amplitudes for lopudnesses from 0 to -99 dB.
        Low_pass        Low-pass filter, cutoff at sample_freq / 4.


        All data are fixed-point short integers, in which
        -32768 to +32768 represent -1.0 to +1.0. Integer arithmetic
        is used for speed, instead of the more natural floating-point.

        For the forward FFT (time -> freq), fixed scaling is
        performed to prevent arithmetic overflow, and to map a 0dB
        sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
        coefficients; the one in the lower half is reported as 0dB
        by fix_loud(). The return value is always 0.

        For the inverse FFT (freq -> time), fixed scaling cannot be
        done, as two 0dB coefficients would sum to a peak amplitude of
        64K, overflowing the 32k range of the fixed-point integers.
        Thus, the fix_fft() routine performs variable scaling, and
        returns a value which is the number of bits LEFT by which
        the output must be shifted to get the actual amplitude
        (i.e. if fix_fft() returns 3, each value of fr[] and fi[]
        must be multiplied by 8 (2**3) for proper scaling.
        Clearly, this cannot be done within the fixed-point short
        integers. In practice, if the result is to be used as a
        filter, the scale_shift can usually be ignored, as the
        result will be approximately correctly normalized as is.


        TURBO C, any memory model; uses inline assembly for speed
        and for carefully-scaled arithmetic.

        Written by:  Tom Roberts  11/8/89
        Made portable:  Malcolm Slaney 12/15/94 malcolm@interval.com

                Timing on a Macintosh PowerBook 180.... (using Symantec C6.0)
                        fix_fft (1024 points)             8 ticks
                        fft (1024 points - Using SANE)  112 Ticks
                        fft (1024 points - Using FPU)    11

*/

//integer fft library: http://www.jjj.de/crs4/integer_fft.c

#include <math.h>

#define FIX_MPY(DEST,A,B)       DEST = ((long)(A) * (long)(B))>>15
//define N_WAVE          1024    /* dimension of Sinewave[] */
//define LOG2_N_WAVE     10      /* log2(N_WAVE) */
//define N_LOUD          100     /* dimension of Loudampl[] */
//ifndef fixed
//define fixed short
//define fixed int
//endif





/*
        fix_mpy() - fixed-point multiplication
*/
fixed fix_mpy(fixed a, fixed b)
{
        FIX_MPY(a,a,b);
        return a;
}

/*
        iscale() - scale an integer value by (numer/denom)
*/
int iscale(int value, int numer, int denom)
{
  return (long) value * (long)numer/(long)denom;
}

/*
        fix_dot() - dot product of two fixed arrays
*/
//fixed fix_dot(fixed *hpa, fixed *pb, int n)
//{
//        fixed *pa;
//        long sum;
//        register fixed a,b;
//        //unsigned int seg,off;
//
//
//        sum = 0L;
//        while(n--) {
//                a = *pa++;
//                b = *pb++;
//                FIX_MPY(a,a,b);
//                sum += a;
//        }
//
//        if(sum > 0x7FFF)
//                sum = 0x7FFF;
//        else if(sum < -0x7FFF)
//                sum = -0x7FFF;
//
//        return (fixed)sum;
//
//}


/*
        fix_fft() - perform fast Fourier transform.

        if n>0 FFT is done, if n<0 inverse FFT is done
        fr[n],fi[n] are real,imaginary arrays, INPUT AND RESULT.
        size of data = 2**m
        set inverse to 0=dft, 1=idft
*/
int fix_fft(Int_FFT_cfg *cfg, fixed fr[], fixed fi[], int inverse)
{
  int mr,nn,i,j,l,k,istep, n,m,scale, shift;
  //fixed qr,qi,tr,ti,wr,wi,t;
  fixed qr,qi,tr,ti,wr,wi;
  
  int N_WAVE = cfg->N_FFT;
  int LOG2_N_WAVE = cfg->LOG2_N_WAVE;
  //int m = cfg.LOG2_N_WAVE;
  n = cfg->N_FFT;
  
  
  //n = 1<<m;
  
  //if(n > N_WAVE)
  //        return -1;
  
  
  //Serial << "fix_fft: m, n, N_wave: " << m << " " << n << " " << N_WAVE << '\n';
  
  
  mr = 0;
  nn = n - 1;
  scale = 0;
  
  /* decimation in time - re-order data */
  for(m=1; m<=nn; ++m) {
          l = n;
          do {
                  l >>= 1;
          } while(mr+l > nn);
          mr = (mr & (l-1)) + l;
  
          if(mr <= m) continue;
          tr = fr[m];
          fr[m] = fr[mr];
          fr[mr] = tr;
          ti = fi[m];
          fi[m] = fi[mr];
          fi[mr] = ti;
  }
  
  l = 1;
  k = LOG2_N_WAVE-1;
  while(l < n) {
          if(inverse) {
                  /* variable scaling, depending upon data */
                  shift = 0;
                  for(i=0; i<n; ++i) {
                          j = fr[i];
                          if(j < 0)
                                  j = -j;
                          m = fi[i];
                          if(m < 0)
                                  m = -m;
                          if(j > 16383 || m > 16383) {
                                  shift = 1;
                                  break;
                          }
                  }
                  if(shift)
                          ++scale;
          } else {
                  /* fixed scaling, for proper normalization -
                     there will be log2(n) passes, so this
                     results in an overall factor of 1/n,
                     distributed to maximize arithmetic accuracy. */
                  shift = 1;
          }
          /* it may not be obvious, but the shift will be performed
             on each data point exactly once, during this pass. */
          istep = l << 1;
          for(m=0; m<l; ++m) {
                  j = m << k;
                  /* 0 <= j < N_WAVE/2 */
                  wr =  cfg->Sinewave[j+N_WAVE/4];
                  wi = -(cfg->Sinewave[j]);
                  if(inverse)
                          wi = -wi;
                  if(shift) {
                          wr >>= 1;
                          wi >>= 1;
                  }
                  for(i=m; i<n; i+=istep) {
                          j = i + l;
                                  tr = fix_mpy(wr,fr[j]) - fix_mpy(wi,fi[j]);
                                  ti = fix_mpy(wr,fi[j]) + fix_mpy(wi,fr[j]);
                          qr = fr[i];
                          qi = fi[i];
                          if(shift) {
                                  qr >>= 1;
                                  qi >>= 1;
                          }
                          fr[j] = qr - tr;
                          fi[j] = qi - ti;
                          fr[i] = qr + tr;
                          fi[i] = qi + ti;
                  }
          }
          --k;
          l = istep;
  }
  
  return scale;
}


/*      window() - apply a Hanning window       */
void window(Int_FFT_cfg *cfg, fixed fr[], int n)
{
        int i,j,k;
        int N_WAVE = n;

        j = N_WAVE/n;
        n >>= 1;
        for(i=0,k=N_WAVE/4; i<n; ++i,k+=j)
                FIX_MPY(fr[i],fr[i],16384-((cfg->Sinewave[k])>>1));
        n <<= 1;
        for(k-=j; i<n; ++i,k-=j)
                FIX_MPY(fr[i],fr[i],16384-((cfg->Sinewave[k])>>1));
}

/*      fix_loud() - compute loudness of freq-spectrum components.
        n should be ntot/2, where ntot was passed to fix_fft();
        6 dB is added to account for the omitted alias components.
        scale_shift should be the result of fix_fft(), if the time-series
        was obtained from an inverse FFT, 0 otherwise.
        loud[] is the loudness, in dB wrt 32767; will be +10 to -N_LOUD.
*/
//void fix_loud(fixed loud[], fixed fr[], fixed fi[], int n, int scale_shift)
//{
//        int i, max;
//
//        max = 0;
//        if(scale_shift > 0)
//                max = 10;
//        scale_shift = (scale_shift+1) * 6;
//
//        for(i=0; i<n; ++i) {
//                loud[i] = db_from_ampl(fr[i],fi[i]) + scale_shift;
//                if(loud[i] > max)
//                        loud[i] = max;
//        }
//}

/*      db_from_ampl() - find loudness (in dB) from
        the complex amplitude.
*/
//int db_from_ampl(fixed re, fixed im)
//{
//        static long loud2[N_LOUD] = {0};
//        long v;
//        int i;
//
//        if(loud2[0] == 0) {
//                loud2[0] = (long)Loudampl[0] * (long)Loudampl[0];
//                for(i=1; i<N_LOUD; ++i) {
//                        v = (long)Loudampl[i] * (long)Loudampl[i];
//                        loud2[i] = v;
//                        loud2[i-1] = (loud2[i-1]+v) / 2;
//                }
//        }
//
//        v = (long)re * (long)re + (long)im * (long)im;
//
//        for(i=0; i<N_LOUD; ++i)
//                if(loud2[i] <= v)
//                        break;
//
//        return (-i);
//}

//#if N_LOUD != 100
//        ERROR: N_LOUD != 100
//#endif
//fixed Loudampl[100] = {
//  32767,  29203,  26027,  23197,  20674,  18426,  16422,  14636,
//  13044,  11626,  10361,   9234,   8230,   7335,   6537,   5826,
//   5193,   4628,   4125,   3676,   3276,   2920,   2602,   2319,
//   2067,   1842,   1642,   1463,   1304,   1162,   1036,    923,
//    823,    733,    653,    582,    519,    462,    412,    367,
//    327,    292,    260,    231,    206,    184,    164,    146,
//    130,    116,    103,     92,     82,     73,     65,     58,
//     51,     46,     41,     36,     32,     29,     26,     23,
//     20,     18,     16,     14,     13,     11,     10,      9,
//      8,      7,      6,      5,      5,      4,      4,      3,
//      3,      2,      2,      2,      2,      1,      1,      1,
//      1,      1,      1,      0,      0,      0,      0,      0,
//      0,      0,      0,      0,
//};

//ifdef  MAIN



//define M       4
//define N       (1<<M)
//
//main(){
//        fixed real[N], imag[N];
//        int     i;
//
//        for (i=0; i<N; i++){
//                real[i] = 1000*cos(i*2*3.1415926535/N);
//                imag[i] = 0;
//        }
//
//        fix_fft(real, imag, M, 0;
//
//        for (i=0; i<N; i++){
//                printf("%d: %d, %d\n", i, real[i], imag[i]);
//        }
//
//        fix_fft(real, imag, M, 1);
//
//        for (i=0; i<N; i++){
//                printf("%d: %d, %d\n", i, real[i], imag[i]);
//        }
//}
//endif  /* MAIN */


