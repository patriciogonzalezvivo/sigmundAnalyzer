#include "audioInput.h"

#include <iostream>
#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

AudioInput::AudioInput() {
    quit = false;
    pause = false;
    channels = 2;                       // Had to change to stereo for System76 ! (was mono)
    bytes_per_frame = 2 * channels;     // 16-bit
    req_rate = 44100;                   // audio sampling rate in Hz
    frames_per_period = (int)(req_rate/60.0);   // 735 = 44100Hz/60fps assumed
    nperiods = 2;                       // >=2, see ALSA manual
    t_memory = 20.0;                    // memory of our circular buffer in secs

    period_size = frames_per_period * bytes_per_frame;
    chunk = new char[period_size];      // raw data buffer for PCM read: 1 period
    req_size = frames_per_period * nperiods; // ALSA device buffer size (frames)
    b_ind = 0;                          // integer index where to write to in buffer    

    // set up sound card for recording (sets rate, size)
    if( initDevice() < 0 )
        exit(1);

    dt = 1.0 / (float)rate;             // sampling period
    b_size = (int)(t_memory * rate);    // buffer size
    printf("memory buffer size %d samples\n", b_size);

    b = new float[b_size];              // our circular audio buffer
    
    // win_size = 1 << param.twowinsize;     // FFT size
    win_size = 1 << 13;                 // FFT size
    std::cout << "win size: " << win_size << std::endl;

    bwin = new float[win_size];         // windowed recent audio data
    winf = new float[win_size];

    setupWindowFunc(winf, win_size);    // windowing function
    
    // set up fast in-place single-precision real-to-half-complex DFT:
    fftw_p = fftwf_plan_r2r_1d(win_size, bwin, bwin, FFTW_R2HC, FFTW_MEASURE);

    // # freqs   ...spectrogram stuff
    // n_f = 560;
    n_f = 128;
    specslice = new float[n_f];
    
    // # time windows: should be multiple of 4 for glDrawPixels
    // n_tw = 940;                     
    n_tw = 128;
    pixel_channels = 3;
    pixels = new unsigned char[n_f * n_tw * pixel_channels];

    Hz_per_pixel = 1.0F / (win_size*dt);
    printf("Hz per pixel = %.3f\n", Hz_per_pixel);
 
    // Start recording thread... runs independently, writing data into ai->b
    pthread_create(&capture_thread, NULL, audioCapture, (void*)this); // this?
}

AudioInput::~AudioInput() {
    snd_pcm_close (pcm_handle);
    fftwf_destroy_plan(fftw_p);
}

void AudioInput::setupWindowFunc(float *w, int N) {
    float W;
    int i;
    // if (verb) printf("windowtype=%d\n", param.windowtype);
    
    // switch (param.windowtype) {
    switch (2) {
        case 0:  // no window (crappy frequency spillover)
            for( i=0; i<N; ++i)
                w[i] = 1.0F;
            break;
        case 1:  // Hann window (C^1 cont, so third-order tails)
            W = N/2.0F;
            for( i=0; i<N; ++i)
                w[i] = (1.0F + cos(M_PI*(i-W)/W))/2;
            break;
        case 2:  // truncated Gaussian window (Gaussian tails + exp small error)
            W = N/5.0F;    // width: keep small truncation but wide to not waste FFT
            for( i=0; i<N; ++i) 
                w[i] = exp(-(i-N/2)*(i-N/2)/(2*W*W));
            break;
        default:
            fprintf(stderr, "unknown windowtype!\n");
    }
}

int AudioInput::initDevice() {  // ........ set up sound card for recording ........
    // ALSA tutorial, taken from http://www.suse.de/~mana/alsa090_howto.html
    
    stream = SND_PCM_STREAM_CAPTURE;
    /* Init pcm_name. Of course, later you */
    /* will make this configurable ;-)     */
    pcm_name = strdup("plughw:0,0");
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);
    /* Open PCM. The last parameter of this function is the mode. */
    /* If this is set to 0, the standard mode is used. Possible   */
    /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */ 
    /* If SND_PCM_NONBLOCK is used, read / write access to the    */
    /* PCM device will return immediately. If SND_PCM_ASYNC is    */
    /* specified, SIGIO will be emitted whenever a period has     */
    /* been completely processed by the soundcard.                */
    if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
        fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
        return(-1);
    }
    /* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Can not configure this PCM device.\n");
        return(-1);
    }
    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error setting access.\n");
        return(-1);
    }
    
    /* Set sample format */
    if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
        fprintf(stderr, "Error setting format.\n");
        return(-1);
    }
    
    /* Set sample rate. If the requested rate is not supported */
    /* by the hardware, use nearest possible rate.         */ 
    rate = req_rate;
    if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, (uint*)&rate, 0) < 0) {
        fprintf(stderr, "Error setting rate.\n");
        return(-1);
    }
    if (rate != req_rate) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n \
                        ==> Using %d Hz instead.\n", req_rate, rate);
    }
    
    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels) < 0) {
        fprintf(stderr, "Error setting channels.\n");
        return(-1);
    }
    
    /* Set number of periods. Periods used to be called fragments. */ 
    if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, nperiods, 0) < 0) {
        fprintf(stderr, "Error setting number of periods.\n");
        return(-1);
    }
    /* Set buffer size (in frames). The resulting latency is given by */
    /* latency = period_size * nperiods / (rate * bytes_per_frame)     */
    size = req_size;
    if (snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &size) < 0) {
        fprintf(stderr, "Error setting buffersize.\n");
        return(-1);
    }

    if( size != req_size ) {
        fprintf(stderr, "Buffer size %d is not supported, using %d instead.\n", (int)req_size, (int)size);
    }
    
    /* Apply HW parameter settings to PCM device and prepare device  */
    if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Error setting HW params.\n");
        return(-1);
    }
    return 1;
}
  
void AudioInput::quitNow() {
    quit = true;
    //      pthread_kill_other_threads_np();
    snd_pcm_close (pcm_handle);
}
  
union Byte {                    // used to convert from signed to unsigned
    unsigned char uchar_val;
    char char_val;
};
  
int AudioInput::mod( int i ) {  // true modulo (handles negative) into our buffer b
    // wraps i to lie in [0, (b_size-1)]. rewritten Barnett
    int r = i % b_size;
    if (r<0)
      r += b_size;
    return r;
}
  
void* AudioInput::audioCapture(void* a) { //-------- capture: thread runs indep --

    // still mostly Luke's code, some names changed. Aims to read 1 "period"
    // (ALSA device setting) into the current write index of our ai->b buffer.
    fprintf(stderr, "audioCapture thread started...\n");
    AudioInput* ai = (AudioInput*) a;  // shares data with main thread = cool!
    
    float inv256 = 1.0 / 256.0;
    float inv256_2 = inv256*inv256;
    
    while( ! ai->quit ) {  // loops around until state of ai kills it
        int n;
        if( ! ai->pause ) {

            // keep trying to get exactly 1 "period" of raw data from sound card...
            while ((n = snd_pcm_readi(ai->pcm_handle, ai->chunk, ai->frames_per_period)) < 0 ) {
                //	  if (n == -EPIPE) fprintf(stderr, "Overrun occurred: %d\n", n); // broken pipe
                fprintf(stderr, "Error occured while recording: %s\n", snd_strerror(n));

                //n = snd_pcm_recover(ai->pcm_handle, n, 0); // ahb

                //fprintf(stderr, "Error occured while recording: %s\n", snd_strerror(n));
                snd_pcm_prepare(ai->pcm_handle);
                //fprintf(stderr, "Dropped audio data (frames read n=%d)\n", n);
            }  // n samples were got

            // if (verb>1) 
            //     printf("snd_pcm_readi got n=%d frames\n", n);

            Byte byte;
            int write_ptr, read_ptr;
            // read chunk into our buffer ai->b ...
            for (int i = 0; i < n; i++ ) {
                read_ptr = i * ai->bytes_per_frame;

                // wraps around
                write_ptr = ai->mod(ai->b_ind + i);
                byte.char_val = ai->chunk[read_ptr];

                // compute float in [-1/2,1/2) from 16-bit raw... (LSB unsigned char)
                ai->b[write_ptr] = (float)ai->chunk[read_ptr+1]*inv256 + (float)byte.uchar_val*inv256_2;
            }

            // update index (in one go)
            ai->b_ind = ai->mod(ai->b_ind+n);

	        // compute spectral slice of recent buffer history
            {
                int N = ai->win_size;       // transform length
                int nf = ai->n_f;           // # freqs to fill in powerspec

                // # freqs   ...spectrogram stuff
                for ( int i=0; i<N; ++i )
                    ai->bwin[i] = ai->winf[i] * ai->b[ai->mod(ai->b_ind - N + i)];

                // do the already-planned fft
                fftwf_execute(ai->fftw_p);

                if (nf>N/2) {
                    fprintf(stderr,"window too short cf n_f!\n"); 
                    return;
                }

                // zero-freq has no imag
                ai->specslice[0] = ai->bwin[0]*ai->bwin[0];

                // compute power spectrum from hc dft
                for ( int i=1; i<nf; ++i )
                    ai->specslice[i] = ai->bwin[i]*ai->bwin[i] + ai->bwin[N-i]*ai->bwin[N-i];
            }
        }
        else
	        usleep(10000);  // wait 0.01 sec if paused (keeps thread CPU usage low)
    }

    fprintf(stderr, "audioCapture thread exiting.\n");
}   