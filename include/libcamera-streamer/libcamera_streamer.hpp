#ifndef LIBCAMERA_STREAMER_H
#define LIBCAMERA_STREAMER_H

#include <thread>

#include "../../src/h264_encoder.hpp"
#include "../../src/camera_wrapper.hpp"
#include "streamer_configuration.hpp"
#include <uvgrtp/context.hh>
#include <uvgrtp/media_stream.hh>

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
    LibcameraStreamer(StreamerConfiguration configuration);
    ~LibcameraStreamer();

    void Start();

private:
    void completedRequestsProcessor() const;
    void encodedFramesProcessor() const;
    void inputBufferProcessedCallback() const;
};

#endif