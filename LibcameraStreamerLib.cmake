# Dirty build a library libcamera-streamer

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

add_library(libcamera-streamer) # initialized below
add_library(libcamera-streamer::libcamera-streamer ALIAS libcamera-streamer)

#----------------------------------------------------------------------------------------------------------------------
# dependencies
#----------------------------------------------------------------------------------------------------------------------
add_subdirectory(readerwriterqueue)

find_package(spdlog REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCAMERA REQUIRED libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")
include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS})
#----------------------------------------------------------------------------------------------------------------------
# sources
#----------------------------------------------------------------------------------------------------------------------

set(public_headers
        include/libcamera-streamer/camera_options.hpp
        include/libcamera-streamer/encoder_options.hpp
        include/libcamera-streamer/libcamera_streamer.h
        include/libcamera-streamer/output_options.hpp
        include/libcamera-streamer/streamer_configuration.hpp
        )

set(sources
        ${public_headers}

        src/h264_encoder.cpp
        src/h264_encoder.h

        src/libcamera_streamer.cpp

        src/camera_wrapper.h
        src/camera_wrapper.cpp

        src/stream_info.hpp
        src/output_item.hpp
        )

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

target_sources(libcamera-streamer PRIVATE ${sources})
target_include_directories(libcamera-streamer
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source")
set_target_properties(libcamera-streamer PROPERTIES
        PUBLIC_HEADER "${public_headers}")
target_compile_features(libcamera-streamer PUBLIC cxx_std_17)

target_link_libraries(libcamera-streamer PRIVATE atomic)
target_link_libraries(libcamera-streamer PUBLIC ${LIBCAMERA_LINK_LIBRARIES} pthread readerwriterqueue fmt uvgrtp)