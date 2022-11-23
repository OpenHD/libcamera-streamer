#ifndef CAMERA_WRAPPER_H
#define CAMERA_WRAPPER_H
#include <queue>
#include "readerwriterqueue/readerwriterqueue.h"

#include <libcamera/libcamera.h>

#include "stream_info.hpp"
#include "libcamera-streamer/camera_options.hpp"

class CameraWrapper
{
private:
    std::unique_ptr<libcamera::CameraManager> cameraManager_;
    std::shared_ptr<libcamera::Camera> camera_;
    libcamera::ControlList controls_;
    CameraOptions *options_;
    std::queue<libcamera::FrameBuffer *> frame_buffers_;
    std::map<libcamera::FrameBuffer *, std::vector<libcamera::Span<uint8_t>>> mapped_buffers_;
    std::vector<std::unique_ptr<libcamera::Request>> requests_;
    std::unique_ptr<libcamera::CameraConfiguration> configuration_;
    libcamera::FrameBufferAllocator *allocator_ = nullptr;

    moodycamel::BlockingReaderWriterQueue<libcamera::Request *> completedRequestsQueue_;
    moodycamel::BlockingReaderWriterQueue<libcamera::Request *> requestsToReuseQueue_;

public:
    CameraWrapper(std::unique_ptr<libcamera::CameraManager> cameraManager, std::string const &cameraId,
                  CameraOptions *options);
    ~CameraWrapper();

    void StartCamera();
    void StopCamera();
    libcamera::Request *WaitForCompletedRequest();
    StreamInfo GetStreamInfo();
    libcamera::FrameBuffer *GetFrameBufferForRequest(const libcamera::Request *request) const;
    std::vector<libcamera::Span<uint8_t>> Mmap(libcamera::FrameBuffer *buffer) const;
    void ReuseRequest();

private:
    void makeRequests();
    void requestComplete(libcamera::Request *request);
    void allocateBuffers();
};
#endif