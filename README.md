# libcamera-streamer

RTP streamer based on libcamera and uvgRTP

## Requirements to build

Install main libs
```
sudo apt install libcamera-dev libspdlog-dev
git clone https://github.com/ultravideo/uvgRTP
```

Install uvgRTP
```
cd uvgRTP
mkdir build && cd build
cmake -DDISABLE_CRYPTO=1 ..
make
sudo make install

I had to remove WError flag from uvgRTP cmake on rpi
```