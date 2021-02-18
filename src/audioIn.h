#pragma once

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <fftw3.h>

enum WindowFunctionType {
    NO_WINDOW = 0,      // no window (crappy frequency spillover)
    HANN_WINDOW = 1,    // Hann window (C^1 cont, so third-order tails)
    TRUNCATED_GAUSSIAN_WINDOW = 2 // truncated Gaussian window (Gaussian tails + exp small error)
};

class AudioIn {
public:
    AudioIn(const char *_device, int number_of_frequencies, int sample_rate = 44100, int channels = 2);
    virtual ~AudioIn();
  
    int     getTotalFreq() { return m_nFreq; }
    float   getHzPerPixel();
    void    quitNow();

    float   *slice;

    bool    pause;
private:
    int     init(const char *_device, snd_pcm_uframes_t _buffer_size, int nperiod);
    static void* audioCapture(void* a);

    float*      m_buffer;
    int         m_buffer_size;
    int         m_buffer_index;

    float*      m_windowed_buffer;
    float*      m_window_functions;
    int         m_window_size;

    int         m_nFreq; 
    int         m_sample_rate;
    int         m_sample_rate_requested;
  
    pthread_t   m_thread;
    fftwf_plan  m_fftw;

    // Audio device data (modified from ALSA tutorial)
    int         m_channels;
    int         m_bytes_per_frame;
    int         m_frames_per_period;

    snd_pcm_uframes_t   m_alsa_buffer_size;  // requested and actual ALSA buffer size
    snd_pcm_t*          m_pcm_handle;        /* Handle for the PCM device */ 
    
    /* Name of the PCM device, like plughw:0,0          */
    /* The first number is the number of the soundcard, */
    /* the second number is the number of the device.   */
    char *              m_pcm_name;

    bool                m_quit;
};
