cmake_minimum_required(VERSION 3.18)
project(libcamera-streamer-example
    VERSION 1.0.0
    DESCRIPTION "Libcamera streamer"
    LANGUAGES CXX)

include(LibcameraStreamerLib.cmake)

#----------------------------------------------------------------------------------------------------------------------
# examples
#----------------------------------------------------------------------------------------------------------------------

add_executable(libcamera-streamer_exe examples/main.cpp)
add_executable(libcamera-streamer::exe ALIAS libcamera-streamer_exe)

set_property(TARGET libcamera-streamer_exe PROPERTY OUTPUT_NAME libcamera-streamer-app)

target_compile_features(libcamera-streamer_exe PRIVATE cxx_std_17)

target_link_libraries(libcamera-streamer_exe PRIVATE libcamera-streamer::libcamera-streamer)
