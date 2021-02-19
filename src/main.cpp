
#include "tools.h"

#include "audioIn.h"
#include "videoOut.h"

#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

static double   elapsedTime = 0.0;
static float    restSeconds = 1.0/60.0;

static float    intensity_offset = 100.f;   // intensity offset
static float    intensity_slope = 2.125f;   // intensity slope (per dB units) 255/120.0

void printUsage(char * executableName) {
    std::cerr << "// Sigmund Analizer by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )" << std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// A companion console program that creates a FFT waterfall from the mic and stream it in as a loopback camera device. "<< std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Usage:" << std::endl;
    std::cerr << executableName << " <dev_video> [--frequencies <width>] [--waterfall_lengh <height>]" << std::endl;
}

void scroll(float *_slice, unsigned char* _pixels, int _w, int _h) {
    // Scroll spectrogram data 1 pixel in t direction, add new col & make 8bit
    int x, y, i, ii;

    // float: NB x (ie t) is the fast storage direc
    for (x = 0; x < _w; ++x) {
        for (y = 0; y < _h - 1; ++y) {
            i = (y * _w + x) * 3;
            ii = ((y + 1) * _w + x) * 3;
            _pixels[i] = _pixels[ii];
            _pixels[i + 1] = _pixels[ii + 1];
            _pixels[i + 2] = _pixels[ii + 2];
        }
    }
    
    float fac = 20.0 * intensity_slope;
    for (x = 0; x < _w; ++x) {
        i = ((_h - 1) * _w + x) * 3;
        float k = (intensity_offset + fac * log10( _slice[x] ));
        
        // Clamp
        if (k > 255.0) 
            k = 255.0; 
        else if (k < 0.0) 
            k = 0.0;

        hue(k/255.0f, _pixels[i], _pixels[i + 1], _pixels[i + 2]);
    }
}

// ===========================================================================
int main(int argc, char** argv) {

    std::string video_path = "";
    std::string audio_path = "plughw:0,0";
    int frequencies = 512;
    int waterfall_lenght = 256;

    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (argument == "--frequencies" ||
            argument == "-w" ) {
            if(++i < argc)
                frequencies = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number of frequencies to analize (X axis). Skipping argument." << std::endl;
        }
        else if (   argument == "--waterfall_length" ||
                    argument == "-h" ) {
            if(++i < argc)
                waterfall_lenght = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number of the lenght of the waterfall (Y axis). Skipping argument." << std::endl;
        }
        else if (   argument == "--intensity_offset" ) {
            if(++i < argc)
                intensity_offset = toFloat(std::string(argv[i]));
        }
        else if (   argument == "--intensity_slope" ) {
            if(++i < argc)
                intensity_slope = toFloat(std::string(argv[i]));
        }
        else if (   argument == "--audio" ||
                    argument == "-i" ) {
            if(++i < argc)
                audio_path = toFloat(std::string(argv[i]));
        }
        else {
            video_path = argument;
        }
    }

    if (video_path.size() == 0) {
        printUsage(argv[0]);
        exit(2);
    }

    AudioIn *audio_in = new AudioIn(audio_path.c_str(), frequencies, 44100, 2);
    VideoOut *video_out = new VideoOut();
    video_out->start(video_path.c_str(), frequencies, waterfall_lenght);

    unsigned char *pixels = new unsigned char[frequencies * waterfall_lenght * 3];

    int scroll_fac = 2;
    int scroll_count = 0;
    while (true) {
        if (!audio_in->pause) {  // scroll every scroll_fac vSyncs, if not paused:
            ++scroll_count;
            if (scroll_count == scroll_fac) {
                scroll(audio_in->slice, pixels, frequencies, waterfall_lenght);   // add spec slice to sg & scroll
                scroll_count = 0;
                if (!video_out->send(pixels))
                    exit(2);
            }
        }

        // Force 60FPS
        double now = getTimeSec();
        float diff = now - elapsedTime;
        if (diff < restSeconds)
            usleep(int((restSeconds - diff) * 1000000));
        elapsedTime = now;
    }

    return 0;
}
