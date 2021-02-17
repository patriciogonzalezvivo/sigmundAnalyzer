#pragma once

struct Param {           // parameters object
    int windowtype;        // DFT windowing function type
    int twowinsize;        // power of 2 giving DFT win_size (N) in samples
};                       // note trailing ;