#pragma once
#include <string>

struct EncoderOptions
{
    unsigned int width;
    unsigned int height;

    float framerate;

    // Set the video bitrate for encoding, in bits/second
    uint32_t bitrate = 0;

    // Set the encoding profile
    std::string profile = "baseline";

    // Set the encoding level
    std::string level = "1";

    //Set the intra frame period
    unsigned int intra = 0;

    // Force PPS/SPS header with every I frame (h264 only)
    bool inline_headers = true;
};
