#ifndef LIBCAMERA_STREAMER_H
#define LIBCAMERA_STREAMER_H

#include <thread>

#include <uvgrtp/context.hh>
#include <uvgrtp/media_stream.hh>
#include "../../src/camera_wrapper.h"
#include "../../src/h264_encoder.h"
#include "streamer_configuration.hpp"

class LibcameraStreamer
{
private:
    std::unique_ptr<CameraWrapper> cameraWrapper_;
    std::unique_ptr<H264Encoder> encoderWrapper_;
    //std::unique_ptr<libcamera::CameraManager> camera_manager_;
    StreamerConfiguration configuration_;
    std::thread fromCameraToEncoderThread_;
    std::thread fromEncoderToOutputThread_;

    uvgrtp::context ctx_;
    uvgrtp::session *sess_;
    uvgrtp::media_stream *stream_;

public:
    explicit LibcameraStreamer(StreamerConfiguration configuration);
    ~LibcameraStreamer();

    void Start();

private:
    void completedRequestsProcessor() const;
    void encodedFramesProcessor() const;
    void inputBufferProcessedCallback() const;
};

#endif