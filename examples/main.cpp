#include <string>
#include "spdlog/spdlog.h"

#include "libcamera-streamer/libcamera_streamer.h"
#include "libcamera-streamer/streamer_configuration.hpp"

auto main() -> int
{
    //spdlog::set_level(spdlog::level::trace);
    // used to configure camera and encoder
    /*const int width=1280;
    const int height=720;
    const int framerate=60;*/
    const int width=640;
    const int height=480;
    const int framerate=90;

    StreamerConfiguration configuration;
    configuration.Camera.width = width;
    configuration.Camera.height = height;
    configuration.Camera.framerate = framerate;
    configuration.Camera.denoise = "off";
    //configuration.Output.Ip = "192.168.88.115";
    //configuration.Output.Ip = "127.0.0.1";
    configuration.Output.Ip ="10.42.0.1";
    configuration.Output.Port = 5600;
    configuration.Encoder.bitrate = 5000000;
    configuration.Encoder.framerate = framerate;
    configuration.Encoder.width = width;
    configuration.Encoder.height = height;

    const auto streamer = std::make_unique<LibcameraStreamer>(configuration);
    streamer->Start();

    while (true)
    {
    }
    return 0;
}
