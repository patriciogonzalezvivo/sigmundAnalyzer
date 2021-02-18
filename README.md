# sigmundAnalizer

Real-time audio spectrogram to loopback v4l

### Installation

You will need to install the following dependencies:

* [v4l2LoopBack](https://github.com/umlaeute/v4l2loopback) this is not require to compile but is crucial for sigmund to stream the data to a video device
* FFTW3
* Alsa

Assuming an ubuntu linux system you shuld install those dependencies doing this:

```
sudo apt-get install libfftw3-dev libasound2 libasound2-dev v4l2loopback-dkms v4l2loopback-utils
```

Clone, compile and install:

```
git clone https://github.com/patriciogonzalezvivo/sigmundAnalizer.git
cd  sigmundAnalizer
make
sudo make install
```

### Usage

First you need to create a v4l2loopback device.

```
// Create a loopback video device in /dev/video9
sudo modprobe v4l2loopback video_nr=9 card_label="loopback"

// Check that is there
ls /dev/video*
```

Then you can run the program pointing that device as destination:

```
signmund /dev/video9
```

Then you can use GlslViewer to read from `/dev/video9` and visualize the data as you prefere:

```
// As a regular FFT bars
glslViewer shaders/bars.frag /dev/video9 -l 


// Waterfall 
glslViewer shaders/waterfall.frag /dev/video9 -l 
```


### TODO

* bound the main loop to a frame rate
* parse through so user can define: device out, width, height and fft parameters
* Add octave analizer, beat detection
* Add OSC for less often data... like octaves peaks and avarage