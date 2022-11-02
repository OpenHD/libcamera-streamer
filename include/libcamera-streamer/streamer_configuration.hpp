#pragma once

#include "output_options.hpp"
#include "encoder_options.hpp"
#include "camera_options.hpp"

struct StreamerConfiguration
{
    CameraOptions Camera;
    EncoderOptions Encoder;
    OutputOptions Output;
};
