#ifndef ENCODER_OPTIONS_H
#define ENCODER_OPTIONS_H

#include <string>
#include <linux/v4l2-controls.h>

struct EncoderOptions
{
    unsigned int width;
    unsigned int height;

    float framerate;

    // Set the video bitrate for encoding, in bits/second
    uint32_t bitrate = 0;

    // Set the encoding profile
    v4l2_mpeg_video_h264_profile profile = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN;

    // Set the encoding level
    v4l2_mpeg_video_h264_level level = V4L2_MPEG_VIDEO_H264_LEVEL_4_0;

    //Set the intra frame period
    unsigned int intra = 30;

    // Force PPS/SPS header with every I frame (h264 only)
    bool inline_headers = true;
};

#endif