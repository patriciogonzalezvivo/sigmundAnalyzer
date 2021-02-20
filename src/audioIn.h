#pragma once

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <functional>

#include "buffer.h"

class AudioIn {
public:
    AudioIn();
    virtual ~AudioIn();
  
    bool        start(const char *_device, std::function<void(Buffer*)> _callback, int sample_rate = 44100, int channels = 2);

    int         getFramesPerPeriod() const { return m_frames_per_period; }
    int         getBytesPerFrame() const { return m_bytes_per_frame; }
    int         getPeriodSize() const { return m_frames_per_period * m_bytes_per_frame; }
    int         getTotalChannels() const { return m_channels; }
    int         getSampleRate() const { return m_sample_rate; }

    Buffer*     getBuffer() { return m_buffer; }

    void        quitNow();
    bool        pause;
private:
    int         openDevice(const char *_device, int _sample_rate, snd_pcm_uframes_t _buffer_size, int nperiod);

    static void* audioCapture(void* a);
    
    // Audio device data (modified from ALSA tutorial)
    int         m_channels;
    int         m_bytes_per_frame;
    int         m_frames_per_period;
    int         m_sample_rate;
  
    snd_pcm_t*  m_pcm_handle;        /* Handle for the PCM device */ 
    pthread_t   m_thread;

    Buffer*     m_buffer;
    std::function<void(Buffer*)> m_callback;

    /* Name of the PCM device, like plughw:0,0          */
    /* The first number is the number of the soundcard, */
    /* the second number is the number of the device.   */

    char*       m_pcm_name;
    bool        m_quit;
};
