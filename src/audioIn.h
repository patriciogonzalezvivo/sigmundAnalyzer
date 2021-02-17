#pragma once

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <fftw3.h>

class AudioIn {
public:
    AudioIn(int nFreq);
    virtual ~AudioIn();
  
    int     initDevice();
    void    setupWindowFunc(float *w, int N);
    void    quitNow();
    int     mod( int i );

    static void* audioCapture(void* a); //-------- capture: thread runs indep --

    char    *chunk;
    float   *b;
    float   *specslice;
    float   *bwin;
    float   *winf;

    int     b_ind;
    int     b_size;
    int     n_f; 
    int     win_size;
    float   dt;
    float   t_memory;
    float   Hz_per_pixel;
  
    bool    quit;
    bool    pause;
    pthread_t capture_thread;
    fftwf_plan fftw_p;

    // Audio device data (modified from ALSA tutorial)
    int bytes_per_frame, frames_per_period, nperiods, channels;
    int req_rate, rate;   /* Requested and actual sample rate */
    int dir;          /* rate == req_rate --> dir = 0 */
                        /* rate < req_rate  --> dir = -1 */
                        /* rate > req_rate  --> dir = 1 */
    snd_pcm_uframes_t   period_size;     // Period size (bytes)
    snd_pcm_uframes_t   req_size, size;  // requested and actual ALSA buffer size
    snd_pcm_t           *pcm_handle;        /* Handle for the PCM device */ 
    snd_pcm_stream_t    stream;     /* Playback stream */
    /* This structure contains information about    */
    /* the hardware and can be used to specify the  */      
    /* configuration to be used for the PCM stream. */ 
    snd_pcm_hw_params_t *hwparams;
    /* Name of the PCM device, like plughw:0,0          */
    /* The first number is the number of the soundcard, */
    /* the second number is the number of the device.   */
    
    char *pcm_name;
};