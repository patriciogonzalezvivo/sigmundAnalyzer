#include "spectrogram.h"

#include <math.h>
#include <iostream>
#include <algorithm>

#include "tools.h"

const char *notenames[] = { "C","C#","D","Eb","E","F","F#","G","G#","A","Bb","B"}; 

Spectrogram::Spectrogram(int _sample_rate, int _window_size, int _number_of_frequencies) {
    // Create a windowed buffer
    m_window_size = _window_size;
    m_windowed_buffer = new float[m_window_size];         // windowed recent audio data
    std::fill(m_windowed_buffer, m_windowed_buffer + sizeof(m_windowed_buffer)*sizeof(float), 0.0f);
    // Create an array of windowing functions
    m_window_functions = new float[m_window_size];
    fillWindowFunc(TRUNCATED_GAUSSIAN_WINDOW, m_window_functions, m_window_size);    // windowing function
    
    float dt = 1.0f / (float)_sample_rate;
    m_hzPixel = 1.0f / (_window_size * dt);

    // set up fast in-place single-precision real-to-half-complex DFT:
    m_fftw = fftwf_plan_r2r_1d(m_window_size, m_windowed_buffer, m_windowed_buffer, FFTW_R2HC, FFTW_MEASURE);

    m_nFreq = _number_of_frequencies;
    m_freqs = new float[m_nFreq];
    std::fill(m_freqs, m_freqs+sizeof(m_freqs)*sizeof(float), 0.0f);
}

Spectrogram::~Spectrogram() {
    // fftwf_destroy_plan(m_fftw);
}

void Spectrogram::update(Buffer* _buffer) {
    int N = m_window_size;       // transform length
    int n = m_nFreq;        // # freqs to fill in powerspec

    int i, ii = 0;

    // # freqs   ...spectrogram stuff
    for (i = 0; i < N; ++i)
        m_windowed_buffer[i] = _buffer->get( -N + i );

    // do the already-planned fft
    fftwf_execute(m_fftw);

    if (n > N/2) {
        std::cerr << "window too short cf " << m_nFreq << std::endl;
        return;
    }

    // zero-freq has no imag
    m_freqs[0] = m_windowed_buffer[0] * m_windowed_buffer[0];

    // compute power spectrum from hc dft
    int max_index = -1;
    float max_value = 0.0f;
    for (i = 1; i < m_nFreq; ++i) {
        ii = N - i;
        m_freqs[i] = m_windowed_buffer[i] * m_windowed_buffer[i] + m_windowed_buffer[ii] * m_windowed_buffer[ii];
        if (m_freqs[i] > max_value) {
            max_value = m_freqs[i];
            max_index = i;
        }
    }
    
    // float max_freq = max_index * m_hzPixel;

    // int midi_note = (int)roundf(12*logf(max_freq/440.0)+69);
    // if (midi_note > 35) {
    //     std::cout << max_freq << "Hz ";

    //     // int notenum = (int)roundf(12 * logf(max_freq/261.626)/logf(2.0));
    //     // int octave = 4 + notenum/12; // relies on int division
    //     // std::cout << notenames[(notenum+1200) % 12] << " ";
    //     // std::cout << octave << " ";

    //     std::cout << midi_note << std::endl;
    // }

}   