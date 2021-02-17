# sigmundAnalizer

Real-time audio spectrogram to loopback v4l

### Installation

This assumes an ubuntu linux system:

```
sudo apt-get install freeglut3 freeglut3-dev libfftw3-dev libasound2 libasound2-dev v4l2loopback-dkms v4l2loopback-utils
sudo modprobe v4l2loopback devices=1 card_label="loopback"
make
sudo make install
```


### Installation
```
sudo modprobe v4l2loopback devices=1 card_label="loopback"
.signmundAnalizer /dev/video4
```

