#include "scene.h"

#include <math.h>
#include <iostream>

Scene::Scene() {
    scroll_fac = 2;
}

Scene::~Scene() {

}

// meaningful constructor - could move some to main to
void Scene::init() {
    // parse cmd line
    pause = false;
    
    color_scale[0] = 20.0; // 8-bit intensity offset
    color_scale[1] = 1.0;//2.125; // 8-bit intensity slope (per dB units)

    ai = new AudioInput();
    fps_tic = time(NULL); 
    frameCount = fps = 0;
    scroll_count = 0;

    gettimeofday(&start_time, NULL);
    run_time = 0.0;
}

float saturate(float value) { 
    return std::max (0.0f, std::min (1.0f, value)); 
}

// http://dystopiancode.blogspot.com/2012/06/yuv-rgb-conversion-algorithms-in-c.html

void Scene::hue(float x, unsigned char& _r, unsigned char& _g, unsigned char& _b ) {
    // color_scale[1] units: intensity/dB
    float fac = 20.0 * color_scale[1];
    int k = (int)(color_scale[0] + fac*log10(x));

    
    // Clamp
    if (k > 255) 
        k = 255; 
    else if (k < 0) 
        k = 0;

    float hue = k/255.0f;       // 0<a<1. now map from [0,1] to rgb in [0,1]
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
    int w = ai->n_f;
    int h = ai->n_tw;
    int c = ai->pixel_channels;

    // float: NB x (ie t) is the fast storage direc
    for (x = 0; x < w; ++x) {
        for (y = 0; y < h - 1; ++y) {
            i = (y * w + x) * c;
            ii = ((y + 1) * w + x) * c;
            ai->pixels[i] = ai->pixels[ii];
            ai->pixels[i + 1] = ai->pixels[ii + 1];
            ai->pixels[i + 2] = ai->pixels[ii + 2];
        }
    }
    
    for (x = 0; x < w; ++x) {
        i = ((h - 1) * w + x) * c;
        hue(ai->specslice[x], ai->pixels[i], ai->pixels[i + 1], ai->pixels[i + 2]);
    }
}

// show spectrogram as 2D pixel array, w/ axes............
void Scene::spectrogram() {
    glDisable(GL_DEPTH_TEST); 
    glDisable(GL_BLEND);
    glDrawPixels(ai->n_f, ai->n_tw, GL_RGB, GL_UNSIGNED_BYTE, ai->pixels);
}