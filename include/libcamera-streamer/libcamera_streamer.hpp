#pragma once
#include <thread>

//#include "../../source/h264_encoder.hpp"
//#include "../../source/camera_wrapper.hpp"
#include "streamer_configuration.hpp"

class LibcameraStreamer
{
private:
//    std::unique_ptr<CameraWrapper> cameraWrapper_;
//    std::unique_ptr<H264Encoder> encoderWrapper_;
    //std::unique_ptr<libcamera::CameraManager> camera_manager_;
    StreamerConfiguration configuration_;
    std::thread mainStreamerThread_;
    std::thread fromCameraToEncoderThread_;

public:
    LibcameraStreamer(StreamerConfiguration configuration);
    ~LibcameraStreamer();

    void Start();

private:
    void EventLoop();
    void CompletedRequestsProcessor();
};
