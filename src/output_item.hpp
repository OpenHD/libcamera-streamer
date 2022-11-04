#pragma once
struct OutputItem
{
    void *mem;
    size_t bytes_used;
    size_t length;
    unsigned int index;
    bool keyframe;
    int64_t timestamp_us;
};