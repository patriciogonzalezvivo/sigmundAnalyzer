#pragma once

#include <sys/time.h>

#include "audioIn.h"
#include "videoOut.h"

class Scene {
public:
    Scene();                    // trivial constructor, to prevent global init fiasco
    virtual ~Scene();           // destructor

    bool init(char * _video, int _nFreq, int _tail);                // meaningful constructor - could move some to main to
    bool update();
    void scroll();

private:
    AudioIn         *m_audio_in;
    VideoOut        *m_video_out;

    unsigned char   *m_pixels;
    int             m_pixel_channels;
    
    time_t          m_fps_tic;              // FPS time reference
    timeval         m_start_time;

    float           m_intensity_offset;     // intensity offset
    float           m_intensity_slope;      // intensity slope (per dB units)

    float           m_run_time;
    int             m_frameCount;
    int             m_fps;
    int             m_scroll_fac;
    int             m_scroll_count;
    bool            m_pause;

};
