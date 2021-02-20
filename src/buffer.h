#pragma once

#include <algorithm>

#include "tools.h"

class Buffer {
public:
    Buffer( int size ){
        m_size = (int)(size);
        m_index = 0;                             // integer index where to write to in buffer    
        // our circular audio buffer
        m_buffer = new float[m_size];
        std::fill(m_buffer, m_buffer+sizeof(m_buffer)*sizeof(float), 0.0f);
    }
    virtual ~Buffer() {};

    int     size() const { return m_size; }

    void    set(int i, const float &_value) { m_buffer[ mod(m_index + i, m_size) ] = _value; }
    float   get(int i) const { return m_buffer[ mod(m_index + i, m_size) ]; }

    void    offsetIndex(int n) { m_index = mod(m_index + n, m_size); }

private:
    float*      m_buffer;
    int         m_size;
    int         m_index;
};