#pragma once

class VideoOut {
public:
    VideoOut();
    virtual ~VideoOut();

    bool    init(char *_device, int _width, int _height);
    bool    send(unsigned char *_pixels, int size);
    bool    send(unsigned char *_pixels);

    int     getWidth() { return m_width; }
    int     getHeight(){ return m_height; }

private:
    int m_width;
    int m_height;

    int m_sink;
    int m_size;
    // char *m_buffer;
};