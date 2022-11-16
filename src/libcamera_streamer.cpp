#include "libcamera-streamer/libcamera_streamer.h"

#include <utility>
#include "spdlog/spdlog.h"
#include <uvgrtp/lib.hh>

#include <chrono>

//#include "completed_request.hpp"
//#include "output/output.hpp"

LibcameraStreamer::LibcameraStreamer(StreamerConfiguration configuration)
    :configuration_(std::move(configuration))
{
    spdlog::trace("LibcameraStreamer streamer creating");
    auto cameraManager = std::make_unique<libcamera::CameraManager>();
    const auto isStarted = cameraManager->start();
    if (isStarted) {
        throw std::runtime_error("camera manager failed to start, code "
            + std::to_string(-isStarted));
    }

    auto cameras = cameraManager->cameras();
    // Do not show USB webcams as these are not supported in libcamera-apps!
    auto rem = std::remove_if(
        cameras.begin(),
        cameras.end(),
        [](auto& cam) { return cam->id().find("/usb") != std::string::npos; });
    cameras.erase(rem, cameras.end());

    if (cameras.empty()) {
        throw std::runtime_error("no cameras available");
    }

    std::string const& cam_id = cameras[0]->id();

    cameraWrapper_ = std::make_unique<CameraWrapper>(std::move(cameraManager), cam_id, &configuration_.Camera);
    auto streamInfo = cameraWrapper_->GetStreamInfo();
    encoderWrapper_ = std::make_unique<H264Encoder>(&configuration_.Encoder, streamInfo,
                                                    [=]() -> void { this->inputBufferProcessedCallback(); });

    sess_ = ctx_.create_session(configuration_.Output.Ip);
    int flags = RCE_SEND_ONLY;
    stream_ = sess_->create_stream(configuration_.Output.Port, RTP_FORMAT_H264, flags);
    stream_->configure_ctx(RCC_MTU_SIZE, 1400);
    spdlog::trace("LibcameraStreamer streamer created");
}

LibcameraStreamer::~LibcameraStreamer() {}

void LibcameraStreamer::Start()
{
    fromCameraToEncoderThread_ = std::thread(&LibcameraStreamer::completedRequestsProcessor, this);
    fromEncoderToOutputThread_ = std::thread(&LibcameraStreamer::encodedFramesProcessor, this);
    cameraWrapper_->StartCamera();
    encoderWrapper_->Start();
}

using namespace std::placeholders;

static uint64_t getTimeNs(){
    const auto time=std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}
static uint64_t __attribute__((unused)) getTimeUs(){
    const auto time=std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(time).count();
}

// called when there is a new libcamera raw buffer
void LibcameraStreamer::completedRequestsProcessor() const
{
    while (true) {
        const auto request = cameraWrapper_->WaitForCompletedRequest();
        spdlog::trace("LibcameraStreamer: New completed request");

        const auto buffer = cameraWrapper_->GetFrameBufferForRequest(request);
        libcamera::Span bufferMemory = cameraWrapper_->Mmap(buffer)[0];
        auto ts = request->metadata().get(libcamera::controls::SensorTimestamp);
        int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
        const auto delay_ns=getTimeNs()-timestamp_ns;
        const float delay_ms=delay_ns / 1000 / 1000.0;
        spdlog::info("Delay camera?: {} ms",delay_ms);
        // feed current time to measure encode only
        encoderWrapper_->EncodeBuffer(buffer->planes()[0].fd.get(), bufferMemory.size(), getTimeUs());
    }
}

void LibcameraStreamer::encodedFramesProcessor() const
{
    while (true)
    {
        auto nextOutputItem = encoderWrapper_->WaitForNextOutputItem();
        const auto delay_us=getTimeUs()-nextOutputItem->timestamp_us;
        const float delay_ms=delay_us / 1000.0;
        spdlog::info("Delay encode: {} ms",delay_ms);
        stream_->push_frame(static_cast<uint8_t *>(nextOutputItem->mem), nextOutputItem->bytes_used, RTP_COPY);
        encoderWrapper_->OutputDone(nextOutputItem);
    }
}

void LibcameraStreamer::inputBufferProcessedCallback() const
{
    spdlog::trace("Streamer received input done");
    cameraWrapper_->ReuseRequest();
}
