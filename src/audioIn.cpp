#include "audioIn.h"

#include <iostream>
#include <math.h>

AudioIn::AudioIn():
    m_quit(false) {
}

AudioIn::~AudioIn() {
    snd_pcm_close( m_pcm_handle );
}


bool AudioIn::start(const char *_device, std::function<void(Buffer*)> _callback, int _sample_rate, int _channels) {

    m_channels = _channels;                         // Had to change to stereo for System76 ! (was mono)
    m_bytes_per_frame = 2 * m_channels;             // 16-bit
    m_frames_per_period = (int)(_sample_rate/60.0f); // 735 = 44100Hz/60fps assumed

    // set up sound card for recording (sets rate, size)
    int nperiods = 2;                               // >=2, see ALSA manual
    if( openDevice(_device, _sample_rate, m_frames_per_period * nperiods, nperiods) < 0 )
        exit(1);

    m_callback = _callback;

    // sampling period
    float t_memory = 20.0f;  // memory of our circular buffer in secs
    m_buffer = new Buffer( (int)(t_memory * m_sample_rate) );
    
    // Start recording thread... runs independently, writing data into ai->b
    pthread_create(&m_thread, NULL, audioCapture, (void*)this); // this?

    std::cout << "Opening audio in at " << _device << " at " << m_sample_rate << std::endl; 

    return true;
}

// ........ set up sound card for recording ........
int AudioIn::openDevice(const char *_device, int _rate_requested, snd_pcm_uframes_t _buffer_size, int nperiods) {  
    // ALSA tutorial, taken from http://www.suse.de/~mana/alsa090_howto.html

    /* This structure contains information about    */
    /* the hardware and can be used to specify the  */      
    /* configuration to be used for the PCM stream. */ 
    snd_pcm_hw_params_t *pcm_hwparams;
    snd_pcm_uframes_t   pcm_buffer_size;
    
    /* Init pcm_name. Of course, later you */
    /* will make this configurable ;-)     */
    m_pcm_name = strdup(_device);

    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&pcm_hwparams);
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
    /* Init pcm_hwparams with full configuration space */
    if (snd_pcm_hw_params_any(m_pcm_handle, pcm_hwparams) < 0) {
        fprintf(stderr, "Can not configure this PCM device.\n");
        return(-1);
    }
    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if (snd_pcm_hw_params_set_access(m_pcm_handle, pcm_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error setting access.\n");
        return(-1);
    }
    
    /* Set sample format */
    if (snd_pcm_hw_params_set_format(m_pcm_handle, pcm_hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
        fprintf(stderr, "Error setting format.\n");
        return(-1);
    }
    
    /* Set sample rate. If the requested rate is not supported */
    /* by the hardware, use nearest possible rate.         */ 
    m_sample_rate = _rate_requested;
    if (snd_pcm_hw_params_set_rate_near(m_pcm_handle, pcm_hwparams, (uint*)&m_sample_rate, 0) < 0) {
        fprintf(stderr, "Error setting rate.\n");
        return(-1);
    }
    if (m_sample_rate != _rate_requested) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n \
                        ==> Using %d Hz instead.\n", _rate_requested, m_sample_rate);
    }
    
    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(m_pcm_handle, pcm_hwparams, m_channels) < 0) {
        fprintf(stderr, "Error setting channels.\n");
        return(-1);
    }
    
    /* Set number of periods. Periods used to be called fragments. */ 
    if (snd_pcm_hw_params_set_periods(m_pcm_handle, pcm_hwparams, nperiods, 0) < 0) {
        fprintf(stderr, "Error setting number of periods.\n");
        return(-1);
    }
    /* Set buffer size (in frames). The resulting latency is given by */
    /* latency = period_size * nperiods / (rate * m_bytes_per_frame)     */
    pcm_buffer_size = _buffer_size;
    if (snd_pcm_hw_params_set_buffer_size_near(m_pcm_handle, pcm_hwparams, &pcm_buffer_size) < 0) {
        fprintf(stderr, "Error setting buffersize.\n");
        return(-1);
    }

    if( pcm_buffer_size != _buffer_size ) {
        fprintf(stderr, "Buffer size %d is not supported, using %d instead.\n", (int)_buffer_size, (int)pcm_buffer_size);
    }
    
    /* Apply HW parameter settings to PCM device and prepare device  */
    if (snd_pcm_hw_params(m_pcm_handle, pcm_hwparams) < 0) {
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

//-------- capture: thread runs indep --
void* AudioIn::audioCapture(void* a) {

    AudioIn* ai = (AudioIn*) a;  // shares data with main thread = cool!
    // std::cout << "Listening audio from " << ai->m_pcm_name << std::endl;;

    // snd_pcm_uframes_t period_size = ai->getFramesPerPeriod() * ai->getBytesPerFrame();
    char *tmp = new char[ai->getPeriodSize()];            // raw data buffer for PCM read: 1 period
    
    float inv256 = 1.0 / 256.0;
    float inv256_2 = inv256*inv256;
    
    while( ! ai->m_quit ) {  // loops around until state of ai kills it
        int n;

        // keep trying to get exactly 1 "period" of raw data from sound card...
        while ((n = snd_pcm_readi(ai->m_pcm_handle, tmp, ai->getFramesPerPeriod())) < 0 ) {
            fprintf(stderr, "Error occured while recording: %s\n", snd_strerror(n));
            snd_pcm_prepare(ai->m_pcm_handle);
        }

        // printf("snd_pcm_readi got n=%d frames\n", n);

        Byte byte;
        int read_ptr;

        // read chunk into our buffer ai->b ...
        for (int i = 0; i < n; i++ ) {
            read_ptr = i * ai->m_bytes_per_frame;

            // wraps around
            byte.char_val = tmp[read_ptr];
            // compute float in [-1/2,1/2) from 16-bit raw... (LSB unsigned char)
            ai->m_buffer->set(i, (float)tmp[read_ptr + 1] * inv256 + (float)byte.uchar_val * inv256_2);
        }

        // update index (in one go)
        ai->m_buffer->offsetIndex(n);

        ai->m_callback(ai->m_buffer);

    }

    fprintf(stderr, "audioCapture thread exiting.\n");
    return NULL;
}   