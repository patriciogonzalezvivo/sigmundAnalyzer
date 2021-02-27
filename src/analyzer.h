#pragma once

#include "buffer.h"
#include <fftw3.h>

#include <string>

class Analyzer {
public:

    Analyzer(int sample_rate, int window_size, int number_of_frequencies );
    virtual ~Analyzer();

    int     getTotalFrequencies() { return m_nFreq; }
    float*  getFrequencies() { return m_freqs; }

    float   getMainFrequency() { return m_mainFreqIndex * m_hzPerFreq; }

    static  int getMidiNoteFor(float freq);
    static  int getOctaveFor(float _freq);
    static  std::string getNoteFor(float freq);

    void    update(Buffer* _buffer);

private:
    fftwf_plan  m_fftw;

    float       m_hzPerFreq;
    float*      m_freqs             = NULL;
    float*      m_windowed_buffer   = NULL;
    float*      m_window_functions  = NULL;
    int         m_window_size;
    int         m_mainFreqIndex;
    
    int         m_nFreq; 
};