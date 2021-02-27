
#include "tools.h"

#include "audioIn.h"
#include "analyzer.h"
#include "videoOut.h"

#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

static double   elapsedTime = 0.0;
static float    restSeconds = 1.0/60.0;

static float    spectogram_intensity_slope = 2.125f; //2.125f;   // intensity slope (per dB units) 255/120.0

static int      scroll_fac = 2;     // how many vSyncs to wait before scrolling sg

void printUsage(char * executableName) {
    std::cerr << "// Sigmund Analizer by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )" << std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// A companion console program that creates a FFT waterfall from the mic and stream it in as a loopback camera device. "<< std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Usage:" << std::endl;
    std::cerr << executableName << " <dev_video> [--frequencies <width>] [--buffer_length <height>]" << std::endl;
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
    
    float fac = 20.0 * spectogram_intensity_slope;
    for (x = 0; x < _w; ++x) {
        i = ((_h - 1) * _w + x) * 3;
        float k = ( fac * log10( _slice[x] )) / 255.0f;
        k = saturate(k);
        hue(k, _pixels[i], _pixels[i + 1], _pixels[i + 2]);
    }
}

// ===========================================================================
int main(int argc, char** argv) {

    int frequencies = 512 * 2;
    std::string audio_path = "plughw:0,0";

    std::string spectogram_video_path = "";
    int spectogram_length = 256 / 2;

    std::string pitch_path = "";

    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (    argument == "--audio" ||
                argument == "-i" ) {
            if (++i < argc)
                audio_path = toFloat(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the ALSA address for the audio in. Using default: " << audio_path << std::endl;
        }
        else if (argument == "--frequencies" ) {
            if (++i < argc)
                frequencies = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number of frequencies to analize. Using default: " << frequencies << std::endl;
        }
        else if (   argument == "--spectogram" ) {
            if (++i < argc)
                spectogram_video_path = std::string(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by the spectogram targeted /dev/videoX file. Skipping argument." << std::endl;
        }
        else if (   argument == "--spectogram_length") {
            if (++i < argc)
                spectogram_length = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number length of the apectogram. Using default: " << spectogram_length << std::endl;
        }
        else if (   argument == "--spectogram_intensity_slope" ) {
            if (++i < argc)
                spectogram_intensity_slope = toFloat(std::string(argv[i]));
        }
        
    }

    AudioIn *audio_in = new AudioIn();
    int window_size = 1 << 13;  // 8192 samples (around 0.19 sec). Remains fixed (min 10, max 18)
    int sample_rate = 44100;
    Analyzer *analyzer = new Analyzer(sample_rate, window_size, frequencies);
    audio_in->start(audio_path.c_str(), [&](Buffer* _buffer){ analyzer->update(_buffer); }, sample_rate, 2);

    // Spectogram OUT as Video
    VideoOut *video_out = nullptr;
    unsigned char *pixels = nullptr;
    if (spectogram_video_path.size() > 0) {
        video_out = new VideoOut();
        video_out->start(spectogram_video_path.c_str(), frequencies, spectogram_length);
        pixels = new unsigned char[frequencies * spectogram_length * 3];
    }

    int scroll_count = 0;
    int prev_midi_note = -1;
    while (true) {

        // Make frame for Video Out
        if (video_out != nullptr) {
            ++scroll_count;
            if (scroll_count == scroll_fac) {
                scroll(analyzer->getFrequencies(), pixels, analyzer->getTotalFrequencies(), spectogram_length);   // add spec slice to sg & scroll
                scroll_count = 0;
                if (!video_out->send(pixels))
                    exit(2);
            }
        }

        float main_freq = analyzer->getMainFrequency();
        int midi_note = analyzer->getMidiNoteFor(main_freq);
        if (midi_note != prev_midi_note) {
            prev_midi_note = midi_note;

            if (midi_note > 0) {
                std::cout << "Pitch change to " << analyzer->getNoteFor(main_freq);
                std::cout << analyzer->getOctaveFor(main_freq) << " ( midi:" << midi_note << " " << main_freq << "Hz)" << std::endl;
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
