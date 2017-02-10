

//from GHA_Demo.c
// DSL prescription - (first subject, left ear) from LD_RX.mat
static CHA_DSL dsl = {5,  //attack (ms)
  50,  //release (ms)
  119, //max dB
  0, // 0=left, 1=right
  8, //num channels
  {317.1666,502.9734,797.6319,1264.9,2005.9,3181.1,5044.7},   // cross frequencies (Hz)
  {-13.5942,-16.5909,-3.7978,6.6176,11.3050,23.7183,35.8586,37.3885},  // compression-start gain
  {0.7,0.9,1,1.1,1.2,1.4,1.6,1.7},   // compression ratio
  {32.2,26.5,26.7,26.7,29.8,33.6,34.3,32.7},    // compression-start kneepoint
  {78.7667,88.2,90.7,92.8333,98.2,103.3,101.9,99.8}  // broadband output limiting threshold
};

//from GHA_Demo.c  from "amplify()"
CHA_WDRC gha = {1.f, // attack time (ms)
  50.f,     // release time (ms)
  24000.f,  // sampling rate (Hz)
  119.f,    // maximum signal (dB SPL)
  0.f,      // compression-start gain
  105.f,    // compression-start kneepoint
  10,     // compression ratio
  105     // broadband output limiting threshold
};

/*
amplify(float *x, float *y, int n, double fs, CHA_DSL *dsl)
{
    int nc;
    static int    nw = 256;         // window size
    static int    cs = 32;          // chunk size
    static int    wt = 0;           // window type: 0=Hamming, 1=Blackman
    static void *cp[NPTR] = {0};
    static CHA_WDRC gha = {1, 50, 24000, 119, 0, 105, 10, 105};

    nc = dsl->nchannel;
    cha_firfb_prepare(cp, dsl->cross_freq, nc, fs, nw, wt, cs);
    cha_agc_prepare(cp, dsl, &gha);
    WDRC(cp, x, y, n, nc);
}
*/

/*
// from: firfb_prepare.c
// called as:    cha_firfb_prepare(cp, dsl->cross_freq, nc, fs, nw, wt, cs);
cha_firfb_prepare(CHA_PTR cp, 
  double *cf, //cross-frequencies (given)
  int nc,     //number of channels (8)
  double fs,  //sample rate (24000)
  int nw,     //window size (256)
  int wt,     //window type (0=Hamming, 1=blackman)
  int cs)     //chunk size (32)
{
    float   *bb;
    int      ns, nt;

    if (cs <= 0) {
        return (1);
    }
    cha_prepare(cp);
    CHA_IVAR[_cs] = cs;  //chunk size
    CHA_DVAR[_fs] = fs;  //sample rate
    // allocate window buffers
    CHA_IVAR[_nw] = nw;  //window size (256, length of FFT?)
    CHA_IVAR[_nc] = nc;  //number of channels
    nt = nw * 2;   //2*256....length of complex values in FFT?
    ns = nt + 2;   //somehow accomodating nyquist
    cha_allocate(cp, ns, sizeof(float), _ffxx);  //allocate memory
    cha_allocate(cp, ns, sizeof(float), _ffyy);  //allocate memory
    cha_allocate(cp, nc * (nw + cs), sizeof(float), _ffzz);  //allocate memory
    // compute FIR-filterbank coefficients
    bb = calloc(nc * nw, sizeof(float));
    fir_filterbank(bb, cf, nc, nw, wt, fs);
    // Fourier-transform FIR coefficients
    if (cs < nw) {  // short chunk
        fir_transform_sc(cp, bb, nc, nw, cs);
    } else {        // long chunk
        fir_transform_lc(cp, bb, nc, nw, cs);
    }
    free(bb);

    return (0);
}
*/
