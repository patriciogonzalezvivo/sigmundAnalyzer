#include "scene.h"
#include "tools.h"

#include <stdlib.h>

#include <sys/time.h>

static double elapsedTime = 0.0;
static float restSeconds = 1.0/60.0;

static Scene scn;   // global scene which contains everything (eg via scn.ai)

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

void printUsage(char * executableName) {
    std::cerr << "// Sigmund Analizer by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )" << std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// A companion console program that creates a FFT waterfall from the mic and stream it in as a loopback camera device. "<< std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Usage:" << std::endl;
    std::cerr << executableName << " <dev_video> [--frequencies <width>] [--waterfall_lengh <height>]" << std::endl;
}   

// ===========================================================================
int main(int argc, char** argv) {
    int deviceIndex = -1;
    int frequencies = 256;
    int waterfall_lenght = 256;

    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (std::string(argv[i]) == "--frequencies" ||
            std::string(argv[i]) == "-w" ) {
            if(++i < argc)
                frequencies = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number of frequencies to analize (X axis). Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "--waterfall_length" ||
                    std::string(argv[i]) == "-h" ) {
            if(++i < argc)
                waterfall_lenght = toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by the number of the lenght of the waterfall (Y axis). Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "--intensity_offset" ) {
            if(++i < argc)
                scn.intensity_offset = toFloat(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "--intensity_slope" ) {
            if(++i < argc)
                scn.intensity_slope = toFloat(std::string(argv[i]));
        }
        else {
            deviceIndex = i;
        }
    }

    if (deviceIndex == -1) {
        printUsage(argv[0]);
        exit(2);
    }

    scn.init(argv[deviceIndex], frequencies, waterfall_lenght);       // true constructor for global scn object

    while (true) {
        if (!scn.update())
            exit(2);

        double now = getTimeSec();
        float diff = now - elapsedTime;
        if (diff < restSeconds)
            usleep(int((restSeconds - diff) * 1000000));
        elapsedTime = now;
    }
    return 0;
}
