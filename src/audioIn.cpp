#include "audioIn.h"

#include <iostream>
#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

void fillWindowFunc(WindowFunctionType _type, float *w, int N) {
    float W;
    int i;
    
    // switch (param.windowtype) {
    switch (_type) {
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
            W = N/5.0F;         // width: keep small truncation but wide to not waste FFT
            for( i=0; i<N; ++i) 
                w[i] = exp(-(i-N/2)*(i-N/2)/(2*W*W));
            break;
        default:
            fprintf(stderr, "unknown windowtype!\n");
    }
}

AudioIn::AudioIn(const char *_device, int _nFreq, int _sampleRate, int _channels): 
    m_quit (false) {

    pause = false;

    m_channels = _channels;                      // Had to change to stereo for System76 ! (was mono)
    m_bytes_per_frame = 2 * m_channels;         // 16-bit
    m_sample_rate_requested = _sampleRate;       // audio sampling rate in Hz

    m_frames_per_period = (int)(m_sample_rate_requested/60.0f);   // 735 = 44100Hz/60fps assumed
    int nperiods = 2;                           // >=2, see ALSA manual
    float t_memory = 20.0;                      // memory of our circular buffer in secs

    m_buffer_index = 0;                          // integer index where to write to in buffer    

    // set up sound card for recording (sets rate, size)
    if( init(_device, m_frames_per_period * nperiods, nperiods) < 0 )
        exit(1);

    // sampling period
    m_buffer_size = (int)(t_memory * m_sample_rate);
    // printf("memory buffer size %d samples\n", m_buffer_size);

    // our circular audio buffer
    m_buffer = new float[m_buffer_size];
    
    m_window_size = 1 << 13;

    m_windowed_buffer = new float[m_window_size];         // windowed recent audio data
    m_window_functions = new float[m_window_size];

    fillWindowFunc(TRUNCATED_GAUSSIAN_WINDOW, m_window_functions, m_window_size);    // windowing function
    
    // set up fast in-place single-precision real-to-half-complex DFT:
    m_fftw = fftwf_plan_r2r_1d(m_window_size, m_windowed_buffer, m_windowed_buffer, FFTW_R2HC, FFTW_MEASURE);

    m_nFreq = _nFreq;
    slice = new float[m_nFreq];

    printf("Hz per pixel = %.3f\n", getHzPerPixel());
 
    // Start recording thread... runs independently, writing data into ai->b
    pthread_create(&m_thread, NULL, audioCapture, (void*)this); // this?
}

float AudioIn::getHzPerPixel() {
    float dt = 1.0 / (float)m_sample_rate;
    return 1.0F / (m_window_size * dt);
}

AudioIn::~AudioIn() {
    snd_pcm_close( m_pcm_handle );
    // fftwf_destroy_plan(m_fftw);
}

// ........ set up sound card for recording ........
int AudioIn::init(const char *_device, snd_pcm_uframes_t _buffer_size, int nperiods) {  
    // ALSA tutorial, taken from http://www.suse.de/~mana/alsa090_howto.html

    /* This structure contains information about    */
    /* the hardware and can be used to specify the  */      
    /* configuration to be used for the PCM stream. */ 
    snd_pcm_hw_params_t *hwparams;
    
    /* Init pcm_name. Of course, later you */
    /* will make this configurable ;-)     */
    m_pcm_name = strdup(_device);

    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);
    /* Open PCM. The last parameter of this function is the mode. */
    /* If this is set to 0, the standard mode is used. Possible   */
    /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */ 
    /* If SND_PCM_NONBLOCK is used, read / write access to the    */
    /* PCM device will return immediately. If SND_PCM_ASYNC is    */
    /* specified, SIGIO will be emitted whenever a period has     */
    /* been completely processed by the soundcard.                */
    if (snd_pcm_open(&m_pcm_handle, m_pcm_name, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        fprintf(stderr, "Error opening PCM device %s\n", m_pcm_name);
        return(-1);
    }
    /* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(m_pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Can not configure this PCM device.\n");
        return(-1);
    }
    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if (snd_pcm_hw_params_set_access(m_pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error setting access.\n");
        return(-1);
    }
    
    /* Set sample format */
    if (snd_pcm_hw_params_set_format(m_pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
        fprintf(stderr, "Error setting format.\n");
        return(-1);
    }
    
    /* Set sample rate. If the requested rate is not supported */
    /* by the hardware, use nearest possible rate.         */ 
    m_sample_rate = m_sample_rate_requested;
    if (snd_pcm_hw_params_set_rate_near(m_pcm_handle, hwparams, (uint*)&m_sample_rate, 0) < 0) {
        fprintf(stderr, "Error setting rate.\n");
        return(-1);
    }
    if (m_sample_rate != m_sample_rate_requested) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n \
                        ==> Using %d Hz instead.\n", m_sample_rate_requested, m_sample_rate);
    }
    
    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(m_pcm_handle, hwparams, m_channels) < 0) {
        fprintf(stderr, "Error setting channels.\n");
        return(-1);
    }
    
    /* Set number of periods. Periods used to be called fragments. */ 
    if (snd_pcm_hw_params_set_periods(m_pcm_handle, hwparams, nperiods, 0) < 0) {
        fprintf(stderr, "Error setting number of periods.\n");
        return(-1);
    }
    /* Set buffer size (in frames). The resulting latency is given by */
    /* latency = period_size * nperiods / (rate * m_bytes_per_frame)     */
    m_alsa_buffer_size = _buffer_size;
    if (snd_pcm_hw_params_set_buffer_size_near(m_pcm_handle, hwparams, &m_alsa_buffer_size) < 0) {
        fprintf(stderr, "Error setting buffersize.\n");
        return(-1);
    }

    if( m_alsa_buffer_size != _buffer_size ) {
        fprintf(stderr, "Buffer size %d is not supported, using %d instead.\n", (int)_buffer_size, (int)m_alsa_buffer_size);
    }
    
    /* Apply HW parameter settings to PCM device and prepare device  */
    if (snd_pcm_hw_params(m_pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Error setting HW params.\n");
        return(-1);
    }
    return 1;
}
  
void AudioIn::quitNow() {
    m_quit = true;
    //      pthread_kill_other_threads_np();
    snd_pcm_close (m_pcm_handle);
}
  
union Byte {                    // used to convert from signed to unsigned
    unsigned char uchar_val;
    char char_val;
};
  
int mod( int i, int base ) {  // true modulo (handles negative) into our buffer b
    // wraps i to lie in [0, (m_buffer_size-1)]. rewritten Barnett
    int r = i % base;
    if (r < 0)
      r += base;
    return r;
}

//-------- capture: thread runs indep --
void* AudioIn::audioCapture(void* a) {

    AudioIn* ai = (AudioIn*) a;  // shares data with main thread = cool!
    // std::cout << "Listening audio from " << ai->m_pcm_name << std::endl;;

    snd_pcm_uframes_t period_size = ai->m_frames_per_period * ai->m_bytes_per_frame;
    char *tmp = new char[period_size];            // raw data buffer for PCM read: 1 period
    
    float inv256 = 1.0 / 256.0;
    float inv256_2 = inv256*inv256;
    
    while( ! ai->m_quit ) {  // loops around until state of ai kills it
        int n;
        if( ! ai->pause ) {

            // keep trying to get exactly 1 "period" of raw data from sound card...
            while ((n = snd_pcm_readi(ai->m_pcm_handle, tmp, ai->m_frames_per_period)) < 0 ) {
                fprintf(stderr, "Error occured while recording: %s\n", snd_strerror(n));
                snd_pcm_prepare(ai->m_pcm_handle);
            }

            // printf("snd_pcm_readi got n=%d frames\n", n);

            Byte byte;
            int write_ptr, read_ptr;

            // read chunk into our buffer ai->b ...
            for (int i = 0; i < n; i++ ) {
                read_ptr = i * ai->m_bytes_per_frame;

                // wraps around
                write_ptr = mod(ai->m_buffer_index + i, ai->m_buffer_size);
                byte.char_val = tmp[read_ptr];

                // compute float in [-1/2,1/2) from 16-bit raw... (LSB unsigned char)
                ai->m_buffer[write_ptr] = (float)tmp[read_ptr+1]*inv256 + (float)byte.uchar_val*inv256_2;
            }

            // update index (in one go)
            ai->m_buffer_index = mod(ai->m_buffer_index+n, ai->m_buffer_size);

            // compute spectral slice of recent buffer history
            {
                int N = ai->m_window_size;       // transform length
                int n = ai->m_nFreq;        // # freqs to fill in powerspec

                // # freqs   ...spectrogram stuff
                for (int i = 0; i < N; ++i)
                    ai->m_windowed_buffer[i] = ai->m_window_functions[i] * ai->m_buffer[ mod(ai->m_buffer_index - N + i, ai->m_buffer_size) ];

                // do the already-planned fft
                fftwf_execute(ai->m_fftw);

                if (n > N/2) {
                    std::cerr << "window too short cf " << n << std::endl;; 
                    return NULL;
                }

                // zero-freq has no imag
                ai->slice[0] = ai->m_windowed_buffer[0] *ai->m_windowed_buffer[0];

                // compute power spectrum from hc dft
                for (int i = 1; i < n; ++i)
                    ai->slice[i] = ai->m_windowed_buffer[i] * ai->m_windowed_buffer[i] + ai->m_windowed_buffer[N-i] * ai->m_windowed_buffer[N-i];
            }
        }
        else
            usleep(10000);  // wait 0.01 sec if paused (keeps thread CPU usage low)
    }

    fprintf(stderr, "audioCapture thread exiting.\n");
    return NULL;
}   