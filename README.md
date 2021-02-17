# sigmundAnalizer

Real-time audio spectrogram to loopback v4l

### Installation

Assuming an ubuntu linux system:

```
sudo apt-get install freeglut3 freeglut3-dev libfftw3-dev libasound2 libasound2-dev v4l2loopback-dkms v4l2loopback-utils
git clone https://github.com/patriciogonzalezvivo/sigmundAnalizer.git
cd  sigmundAnalizer
make
sudo make install
```

### Usage

First you need to create a v4l2loopback device.

```
sudo modprobe v4l2loopback devices=1 card_label="loopback"
signmundAnalizer /dev/video4
```