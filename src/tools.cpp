#include "tools.h"

#include <sstream>
#include <iostream>
#include <sys/time.h>

#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

struct timespec time_start;
double getTimeSec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec temp;
    if ((now.tv_nsec-time_start.tv_nsec)<0) {
        temp.tv_sec = now.tv_sec-time_start.tv_sec-1;
        temp.tv_nsec = 1000000000+now.tv_nsec-time_start.tv_nsec;
    } else {
        temp.tv_sec = now.tv_sec-time_start.tv_sec;
        temp.tv_nsec = now.tv_nsec-time_start.tv_nsec;
    }
    return double(temp.tv_sec) + double(temp.tv_nsec/1000000000.);
}


int toInt(const std::string& _string) {
    int x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

float toFloat(const std::string& _string) {
    float x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
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

int mod( int i, int base ) {  // true modulo (handles negative) into our buffer b
    // wraps i to lie in [0, (m_size-1)]. rewritten Barnett
    int r = i % base;
    if (r < 0)
      r += base;
    return r;
}

void fillWindowFunc(WindowFunctionType _type, float *w, int N) {
    float W = 0.0f;
    int i;
    
    switch (_type) {
        case NO_WINDOW:  
        //crappy frequency spillover
            for( i = 0; i < N; ++i)
                w[i] = 1.0f;
            break;

        case HANN_WINDOW:  
        // C^1 cont, so third-order tails
            W = N/2.0F;
            for( i=0; i<N; ++i)
                w[i] = (1.0f + cos(M_PI*(i-W)/W))/2;
            break;

        case TRUNCATED_GAUSSIAN_WINDOW:  
        // Gaussian tails + exp small error
        // width: keep small truncation but wide to not waste FFT
            W = N/5.0f;
            for( i=0; i<N; ++i) 
                w[i] = exp(-(i-N/2)*(i-N/2)/(2*W*W));
            break;
    }
}