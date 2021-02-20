#pragma once

#include "buffer.h"
#include <fftw3.h>

class Spectrogram {
public:

    Spectrogram( int window_size, int number_of_frequencies );
    virtual ~Spectrogram();

    int     getTotalFrequencies() { return m_nFreq; }
    float*  getFrequencies() { return m_freqs; }

    void    update(Buffer* _buffer);


private:
    fftwf_plan  m_fftw;

    float*      m_freqs             = NULL;
    float*      m_windowed_buffer   = NULL;
    float*      m_window_functions  = NULL;
    int         m_window_size;
    
    int         m_nFreq; 
};