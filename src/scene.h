#pragma once

#include <GL/glut.h>
#include <GL/gl.h>

#include <sys/time.h>

#include "audioInput.h"

class Scene {
public:
    Scene();                    // trivial constructor, to prevent global init fiasco
    virtual ~Scene();           // destructor

    void init();                // meaningful constructor - could move some to main to
    void spectrogram();         // show spectrogram as 2D pixel array, w/ axes
    void scroll();

    void hue(float x, unsigned char& r, unsigned char& g, unsigned char& b );

    AudioInput* ai;
    
    time_t  fps_tic;            // FPS time reference
    timeval start_time;
    float   color_scale[2];     // sg image log color mapping params

    float   run_time;
    int     frameCount, fps;    // frames per sec and counter
    int     scroll_fac, scroll_count;
    bool    pause;

};
