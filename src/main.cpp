#include "scene.h"

#include <stdlib.h>
#include <iostream>

#include <sys/time.h>

static Scene scn;   // global scene which contains everything (eg via scn.ai)

// ===========================================================================
int main(int argc, char** argv) {
    scn.init(argv[1], 256, 256);       // true constructor for global scn object

    while (true) {
        if (!scn.update()) {
            exit(2);
        }
    }
    return 0;
}
