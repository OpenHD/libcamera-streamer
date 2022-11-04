#pragma once

#include <optional>

#include <libcamera/color_space.h>
#include <libcamera/pixel_format.h>

struct StreamInfo
{
    StreamInfo(
        uint32_t width,
        uint32_t height,
        uint32_t stride, 
        std::optional<libcamera::ColorSpace> colorSpace)
    {
        Width = width;
        Height = height;
        Stride = stride;
        ColorSpace = colorSpace;
    }

    uint32_t Width;
    uint32_t Height;
    uint32_t Stride;
    std::optional<libcamera::ColorSpace> ColorSpace;
};
