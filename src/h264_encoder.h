#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include <thread>
#include <functional>

#include "libcamera-streamer/encoder_options.hpp"
#include "stream_info.hpp"
#include "readerwriterqueue/readerwriterqueue.h"
#include "output_item.hpp"

class H264Encoder
{
private:
    struct BufferDescription
    {
        void *mem;
        size_t size;
    };

private:
    static constexpr int OutputBuffersCount = 6;
    static constexpr int CaptureBuffersCount = 12;

    int fd_;
    moodycamel::BlockingReaderWriterQueue<int> availableInputBuffers_;
    moodycamel::BlockingReaderWriterQueue<OutputItem *> outputItemsQueue_;
    BufferDescription buffers_[CaptureBuffersCount];
    std::thread pollThread_;
    std::function<void(void)> inputBufferProcessedCallback_;

public:
    H264Encoder(EncoderOptions const *options, StreamInfo streamInfo,
                std::function<void(void)> inputBufferProcessedCallback);
    ~H264Encoder();

    void Start();
    void EncodeBuffer(int fd, size_t size, int64_t timestamp_us);
    OutputItem* WaitForNextOutputItem();
    void OutputDone(const OutputItem * outputItem) const;

private:
    void setControlValue(uint32_t id, int32_t value, const std::string &errorText) const;

    // This thread just sits waiting for the encoder to finish stuff. It will either:
    // * receive "output" buffers (codec inputs), which we must return to the caller
    // * receive encoded buffers, which we pass to the application.
    void pollEncoder();

    // Getting ready to reuse output(raw frame) buffers
    void pollReadyToReuseOutputBuffers();
    void pollReadyToProcessCaptureBuffers();
};

#endif