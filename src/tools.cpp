#include "tools.h"

#include <sstream>
#include <iostream>
#include <math.h>
#include <sys/time.h>

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