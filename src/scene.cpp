#include "scene.h"

#include <math.h>
#include <iostream>

Scene::Scene():
    intensity_offset(20.0f),
    intensity_slope(1.0f),
    m_audio_in(NULL),
    m_video_out(NULL),
    m_pixels(NULL),
    m_pixel_channels(3),
    m_run_time(0.0f),
    m_frameCount(0),
    m_fps(0),
    m_scroll_fac(2),
    m_scroll_count(0),
    m_pause(false) {
}

Scene::~Scene() {

}

// meaningful constructor - could move some to main to
bool Scene::init(char * _video, int _nFreq, int _tail) {
    m_audio_in = new AudioIn(_nFreq);

    m_video_out = new VideoOut();
    m_video_out->init(_video, _nFreq, _tail);

    m_pixel_channels = 3;
    m_pixels = new unsigned char[_nFreq * _tail * m_pixel_channels];

    gettimeofday(&m_start_time, NULL);

    return true;
}


bool Scene::update() {
    ++m_frameCount;        // compute FPS...
    
    time_t now = time(NULL);     // integer in seconds

    if (now > m_fps_tic) {    // advanced by at least 1 sec?
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fps_tic = now;
    }

    if (!m_audio_in->pause) {  // scroll every scroll_fac vSyncs, if not paused:
        timeval nowe;  // update runtime
        gettimeofday(&nowe, NULL);

        m_run_time += 1/60.0;                // is less jittery, approx true
        m_run_time = fmod(m_run_time, 100.0);// wrap time label after 100 secs

        ++m_scroll_count;
        if (m_scroll_count == m_scroll_fac) {
            scroll();   // add spec slice to sg & scroll
            m_scroll_count = 0;
            if (m_video_out)
                if (!m_video_out->send(m_pixels))
                    return false;
        }
    }

    return true;
}

float saturate(float value) { 
    return std::max (0.0f, std::min (1.0f, value)); 
}

void hue(float hue, unsigned char& _r, unsigned char& _g, unsigned char& _b ) {
    float r = saturate( abs( fmod( hue * 6., 6.) - 3.) - 1. );
    float g = saturate( abs( fmod( hue * 6. + 4., 6.) - 3.) - 1. );
    float b = saturate( abs( fmod( hue * 6. + 2., 6.) - 3.) - 1. );

    #ifdef HSV2RGB_SMOOTH
    r = r*r*(3. - 2. * r);
    g = g*g*(3. - 2. * g);
    b = b*b*(3. - 2. * b);
    #endif

    _r = static_cast<unsigned char> (int(r * 255));
    _g = static_cast<unsigned char> (int(g * 255)); 
    _b = static_cast<unsigned char> (int(b * 255));
}


void Scene::scroll() {
    // Scroll spectrogram data 1 pixel in t direction, add new col & make 8bit
    int x, y, i, ii;
    int w = m_video_out->getWidth();
    int h = m_video_out->getHeight();

    // float: NB x (ie t) is the fast storage direc
    for (x = 0; x < w; ++x) {
        for (y = 0; y < h - 1; ++y) {
            i = (y * w + x) * m_pixel_channels;
            ii = ((y + 1) * w + x) * m_pixel_channels;
            m_pixels[i] = m_pixels[ii];
            m_pixels[i + 1] = m_pixels[ii + 1];
            m_pixels[i + 2] = m_pixels[ii + 2];
        }
    }
    
    float fac = 20.0 * intensity_slope;
    for (x = 0; x < m_audio_in->n_f; ++x) {
        i = ((h - 1) * w + x) * m_pixel_channels;
        float value = m_audio_in->specslice[x];

        int k = (int)(intensity_offset + fac * log10(value));
        // Clamp
        if (k > 255) 
            k = 255; 
        else if (k < 0) 
            k = 0;

        hue(k/255.0f, m_pixels[i], m_pixels[i + 1], m_pixels[i + 2]);
    }
}