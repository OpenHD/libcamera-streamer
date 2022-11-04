#pragma once

#include <queue>

#include "libcamera-streamer/encoder_options.hpp"
#include "stream_info.hpp"
#include "readerwriterqueue/readerwriterqueue.h"

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
    BufferDescription buffers_[CaptureBuffersCount];

public:
    H264Encoder(EncoderOptions const *options, StreamInfo streamInfo);
    ~H264Encoder();

    void EncodeBuffer(int fd, size_t size, int64_t timestamp_us);

private:
    void setControlValue(uint32_t id, int32_t value, const std::string &errorText) const;
};
