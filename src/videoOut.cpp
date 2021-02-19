#include "videoOut.h"

#include <linux/videodev2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

VideoOut::VideoOut():
    m_width(0),
    m_height(0),
    m_sink(-1), 
    m_size(0)
    {

}

VideoOut::~VideoOut() {

}

bool VideoOut::start(const char *_device, int _width, int _height) {
    m_sink = open(_device, O_WRONLY);

    if (m_sink < 0) {
        fprintf(stderr, "Failed to open v4l2sink device. (%s)\n", strerror(errno));
        return false;
    }

    m_width = _width;
    m_height = _height;
    m_size = _width * _height * 3;

    // setup video for proper format
    struct v4l2_format v;
    v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    int t = ioctl(m_sink, VIDIOC_G_FMT, &v);
    if( t < 0 )
        exit(t);

    v.fmt.pix.width = _width;
    v.fmt.pix.height = _height;
    v.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    v.fmt.pix.field = V4L2_FIELD_NONE;
	// v.fmt.pix.bytesperline = linewidth;
	// v.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    v.fmt.pix.sizeimage = m_size;

    t = ioctl(m_sink, VIDIOC_S_FMT, &v);
    if (t < 0 )
        exit(t);

    return true;
}

bool VideoOut::send(unsigned char *_pixels) {
    return (m_size == write(m_sink, _pixels, m_size));
}
