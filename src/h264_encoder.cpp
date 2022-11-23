#include "h264_encoder.h"

#include <fcntl.h>
#include <stdexcept>
#include <linux/videodev2.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>

static int xioctl(int fd, unsigned long ctl, void *arg)
{
    int ret, num_tries = 10;
    do
    {
        ret = ioctl(fd, ctl, arg);
    }
    while (ret == -1 && errno == EINTR && num_tries-- > 0);
    return ret;
}

static int get_v4l2_colorspace(std::optional<libcamera::ColorSpace> const &cs)
{
    if (cs == libcamera::ColorSpace::Rec709)
    {
        return V4L2_COLORSPACE_REC709;
    }
    else if (cs == libcamera::ColorSpace::Smpte170m)
    {
        return V4L2_COLORSPACE_SMPTE170M;
    }
    
    return V4L2_COLORSPACE_SMPTE170M;
}


H264Encoder::H264Encoder(EncoderOptions const *options, StreamInfo streamInfo,std::function<void(void)> inputBufferProcessedCallback)
{
    inputBufferProcessedCallback_ = inputBufferProcessedCallback;

    constexpr char deviceName[] = "/dev/video11";
    fd_ = open(deviceName, O_RDWR, 0);
    if (fd_ < 0)
    {
        throw std::runtime_error("failed to open V4L2 H264 encoder");
    }

    setControlValue(V4L2_CID_MPEG_VIDEO_BITRATE, options->bitrate, "failed to set bitrate");
    setControlValue(V4L2_CID_MPEG_VIDEO_H264_PROFILE, options->profile, "failed to set profile");
    setControlValue(V4L2_CID_MPEG_VIDEO_H264_LEVEL, options->level, "failed to set level");
    setControlValue(V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, options->intra, "failed to set intra period");
    setControlValue(V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER, options->inline_headers ? 1 : 0, "failed to set inline headers");

    v4l2_format outputFormat = {};
    outputFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    outputFormat.fmt.pix_mp.width = streamInfo.Width;
    outputFormat.fmt.pix_mp.height = streamInfo.Height;
    // We assume YUV420 here, but it would be nice if we could do something
    // like info.pixel_format.toV4L2Fourcc();
    outputFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
    outputFormat.fmt.pix_mp.plane_fmt[0].bytesperline = streamInfo.Stride;
    outputFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    outputFormat.fmt.pix_mp.colorspace = get_v4l2_colorspace(streamInfo.ColorSpace);
    outputFormat.fmt.pix_mp.num_planes = 1;
    if (xioctl(fd_, VIDIOC_S_FMT, &outputFormat) < 0)
    {
        throw std::runtime_error("failed to set output format");
    }

    v4l2_format captureFormat = {};
    captureFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    captureFormat.fmt.pix_mp.width = options->width;
    captureFormat.fmt.pix_mp.height = options->height;
    captureFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    captureFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    captureFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    captureFormat.fmt.pix_mp.num_planes = 1;
    captureFormat.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
    captureFormat.fmt.pix_mp.plane_fmt[0].sizeimage = 512 << 10;
    if (xioctl(fd_, VIDIOC_S_FMT, &captureFormat) < 0)
    {
        throw std::runtime_error("failed to set capture format");
    }

    v4l2_streamparm streamParameters = {};
    streamParameters.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    streamParameters.parm.output.timeperframe.numerator = 1000 / options->framerate;
    streamParameters.parm.output.timeperframe.denominator = 1000;
    if (xioctl(fd_, VIDIOC_S_PARM, &streamParameters) < 0)
    {
        throw std::runtime_error("failed to set streamParameters");
    }

    // Request that the necessary buffers are allocated. The output queue
    // (input to the encoder) shares buffers from our caller, these must be
    // DMABUFs. Buffers for the encoded bitstream must be allocated and
    // m-mapped.

    // v4l2 OUTPUT is actually INPUT for raw frames
    v4l2_requestbuffers outputBuffersRequest = {};
    outputBuffersRequest.count = OutputBuffersCount;
    outputBuffersRequest.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    outputBuffersRequest.memory = V4L2_MEMORY_DMABUF;
    if (xioctl(fd_, VIDIOC_REQBUFS, &outputBuffersRequest) < 0)
    {
        throw std::runtime_error("request for output buffers failed");
    }
    spdlog::trace("Got {} output buffers", outputBuffersRequest.count);

    // We have to maintain a list of the buffers we can use when our caller gives
    // us another frame to encode.
    for (unsigned int i = 0; i < outputBuffersRequest.count; i++)
    {
        availableInputBuffers_.enqueue(i);
    }

    // v4l2 CAPTURE buffers is actually OUTPUT buffers with encoded frames
    v4l2_requestbuffers captureBuffersRequest = {};
    captureBuffersRequest.count = CaptureBuffersCount;
    captureBuffersRequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    captureBuffersRequest.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd_, VIDIOC_REQBUFS, &captureBuffersRequest) < 0)
    {
        throw std::runtime_error("request for capture buffers failed");
    }
    spdlog::trace("Got {} capture buffers", captureBuffersRequest.count);
        
    for (unsigned int i = 0; i < captureBuffersRequest.count; i++)
    {
        v4l2_plane planes[VIDEO_MAX_PLANES];
        v4l2_buffer buffer = {};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        buffer.length = 1;
        buffer.m.planes = planes;
        if (xioctl(fd_, VIDIOC_QUERYBUF, &buffer) < 0)
        {
            throw std::runtime_error("failed to capture query buffer " + std::to_string(i));
        }

        buffers_[i].mem = mmap(
            0, 
            buffer.m.planes[0].length, 
            PROT_READ | PROT_WRITE, 
            MAP_SHARED, 
            fd_,
            buffer.m.planes[0].m.mem_offset);
        if (buffers_[i].mem == MAP_FAILED)
        {
            throw std::runtime_error("failed to mmap capture buffer " + std::to_string(i));
        }

        buffers_[i].size = buffer.m.planes[0].length;
        // Whilst we're going through all the capture buffers, we may as well queue
        // them ready for the encoder to write into.
        if (xioctl(fd_, VIDIOC_QBUF, &buffer) < 0)
        {
            throw std::runtime_error("failed to queue capture buffer " + std::to_string(i));
        }
    }
    
     // Enable streaming and we're done.
    
     v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
     if (xioctl(fd_, VIDIOC_STREAMON, &type) < 0)
     {
         throw std::runtime_error("failed to start output streaming");
     }

     type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     if (xioctl(fd_, VIDIOC_STREAMON, &type) < 0)
     {
         throw std::runtime_error("failed to start capture streaming");
     }
     spdlog::trace("H264Encoder: Codec streaming started");
}

H264Encoder::~H264Encoder() {}

void H264Encoder::Start()
{
     pollThread_ = std::thread(&H264Encoder::pollEncoder, this);
}

void H264Encoder::Stop()
{
    stop_requested= true;
    if(pollThread_.joinable()){
        pollThread_.join();
    }
}

void H264Encoder::EncodeBuffer(int fd, size_t size, int64_t timestamp_us)
{
     spdlog::trace("H264Encoder: EncodeBuffer {} {} {}", fd, size, timestamp_us);
     int index;
     if(!availableInputBuffers_.try_dequeue(index))
     {
         spdlog::warn("H264Encoder: Frame encoding skipped");
         return;
     }
     spdlog::trace("H264Encoder: Using {} buffer", index);

     v4l2_buffer buffer = {};
     v4l2_plane planes[VIDEO_MAX_PLANES] = {};
     buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
     buffer.index = index;
     buffer.field = V4L2_FIELD_NONE;
     buffer.memory = V4L2_MEMORY_DMABUF;
     buffer.length = 1;
     buffer.timestamp.tv_sec = timestamp_us / 1000000;
     buffer.timestamp.tv_usec = timestamp_us % 1000000;
     buffer.m.planes = planes;
     buffer.m.planes[0].m.fd = fd;
     buffer.m.planes[0].bytesused = size;
     buffer.m.planes[0].length = size;
     if (xioctl(fd_, VIDIOC_QBUF, &buffer) < 0)
     {
         throw std::runtime_error("failed to queue input to codec");
     }
}

OutputItem * H264Encoder::WaitForNextOutputItem()
{
     OutputItem *outputItem;
     outputItemsQueue_.wait_dequeue(outputItem);
     return outputItem;
}

void H264Encoder::OutputDone(const OutputItem *outputItem) const
{
     v4l2_buffer buf = {};
     v4l2_plane planes[VIDEO_MAX_PLANES] = {};
     buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     buf.memory = V4L2_MEMORY_MMAP;
     buf.index = outputItem->index;
     buf.length = 1;
     buf.m.planes = planes;
     buf.m.planes[0].bytesused = 0;
     buf.m.planes[0].length = outputItem->length;
     if (xioctl(fd_, VIDIOC_QBUF, &buf) < 0)
     {
         throw std::runtime_error("failed to re-queue encoded buffer");
     }

     delete (outputItem);
}

void H264Encoder::setControlValue(uint32_t id, int32_t value, const std::string &errorText) const
{
    v4l2_control ctrl{};
    ctrl.id = id;
    ctrl.value = value;
    if (xioctl(fd_, VIDIOC_S_CTRL, &ctrl) < 0)
    {
        throw std::runtime_error(errorText);
    }
}

void H264Encoder::pollEncoder()
{
    spdlog::trace("Starting poll thread");
    while (!stop_requested)
    {
        pollfd p = {fd_, POLLIN, 0};
        const int pollResult = poll(&p, 1, 200);
        
        if (pollResult == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            throw std::runtime_error("unexpected errno " + std::to_string(errno) + " from poll");
        }
        if (p.revents & POLLIN)
        {
            pollReadyToReuseOutputBuffers();
            pollReadyToProcessCaptureBuffers();
        }
    }
}

void H264Encoder::pollReadyToReuseOutputBuffers()
{
    v4l2_buffer buffer = {};
    v4l2_plane planes[VIDEO_MAX_PLANES] = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.length = 1;
    buffer.m.planes = planes;
    const int outputRequestResult = xioctl(fd_, VIDIOC_DQBUF, &buffer);
    if (outputRequestResult == 0)
    {
        spdlog::trace("Input buffer {} now available", buffer.index);
        // Return this to the caller, first noting that this buffer, identified
        // by its index, is available for queueing up another frame.
        availableInputBuffers_.enqueue(buffer.index);
        inputBufferProcessedCallback_();
    }
}

void H264Encoder::pollReadyToProcessCaptureBuffers()
{
    v4l2_buffer buffer = {};
    v4l2_plane planes[VIDEO_MAX_PLANES] = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.length = 1;
    buffer.m.planes = planes;
    const int captureRequestResult = xioctl(fd_, VIDIOC_DQBUF, &buffer);
    if (captureRequestResult == 0)
    {
        // We push this encoded buffer to another thread so that our
        // application can take its time with the data without blocking the
        // encode process.
        int64_t timestamp_us = (buffer.timestamp.tv_sec * (int64_t)1000000) + buffer.timestamp.tv_usec;
        OutputItem *item = new OutputItem();
        item->mem = buffers_[buffer.index].mem;
        item->bytes_used = buffer.m.planes[0].bytesused;
        item->length = buffer.m.planes[0].length;
        item->index = buffer.index;
        item->keyframe = !!(buffer.flags & V4L2_BUF_FLAG_KEYFRAME);
        item->timestamp_us = timestamp_us;
        outputItemsQueue_.enqueue(item);
    }
}
