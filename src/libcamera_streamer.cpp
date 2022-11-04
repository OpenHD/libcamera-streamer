#include <utility>
#include "spdlog/spdlog.h"

#include "libcamera-streamer/libcamera_streamer.hpp"

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
    //encoderWrapper_ = std::make_unique<H264Encoder>(&configuration_.Encoder);
    spdlog::trace("LibcameraStreamer streamer created");
}

LibcameraStreamer::~LibcameraStreamer() {}

void LibcameraStreamer::Start()
{
    mainStreamerThread_ = std::thread(&LibcameraStreamer::EventLoop, this);
    fromCameraToEncoderThread_ = std::thread(&LibcameraStreamer::CompletedRequestsProcessor, this);
    cameraWrapper_->StartCamera();
}

using namespace std::placeholders;


void LibcameraStreamer::EventLoop()
{
    // std::unique_ptr<Output> output =
    //     std::unique_ptr<Output>(Output::Create(&configuration_.Output));
    // encoderWrapper_->SetEncodeOutputReadyCallback(
    //     std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
    //
    // encoderWrapper_->OpenCamera();
    // encoderWrapper_->ConfigureVideo();
    // encoderWrapper_->StartEncoder();
    // encoderWrapper_->StartCamera();
    //
    //   for (unsigned int count = 0;; count++) {
    //   LibcameraEncoder::Msg msg = libcameraEncoder_->Wait();
    //   if (msg.type == LibcameraApp::MsgType::Timeout) {
    //     // LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
    //     libcameraEncoder_->StopCamera();
    //     libcameraEncoder_->StartCamera();
    //     continue;
    //   }
    //   if (msg.type == LibcameraEncoder::MsgType::Quit)
    //     return;
    //   else if (msg.type != LibcameraEncoder::MsgType::requestComplete)
    //     throw std::runtime_error("unrecognised message!");
    //   
    //
    //   CompletedRequestPtr& completed_request =
    //       std::get<CompletedRequestPtr>(msg.payload);
    //   libcameraEncoder_->EncodeBuffer(completed_request,
    //                                   libcameraEncoder_->GetStream());
    //}
}

void LibcameraStreamer::CompletedRequestsProcessor() {
    while (true) {
        const auto request = cameraWrapper_->WaitForCompletedRequest();
        spdlog::trace("New completed request");
        const auto buffer = cameraWrapper_->GetFrameBufferForRequest(request);
        libcamera::Span bufferMemory = cameraWrapper_->Mmap(buffer)[0];
        auto ts = request->metadata().get(libcamera::controls::SensorTimestamp);
        int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
        // TODO: �������� ������ � request � ������� ����, ��� ��� ������
        encoderWrapper_->EncodeBuffer(buffer->planes()[0].fd.get(), bufferMemory.size(), timestamp_ns / 1000);
    }
}