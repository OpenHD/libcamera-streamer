#pragma once

#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>
#include <libcamera/transform.h>

struct Mode
{
  Mode()
      : Mode(0, 0, 0, false)
  {
  }
  Mode(unsigned int w, unsigned int h, unsigned int b, bool p)
      : width(w)
      , height(h)
      , bit_depth(b)
      , packed(p)
  {
  }
  Mode(std::string const& mode_string);
  unsigned int width;
  unsigned int height;
  unsigned int bit_depth;
  bool packed;
  libcamera::Size Size() const { return libcamera::Size(width, height); }
  std::string ToString() const;
};

struct CameraOptions
{
  // Options() : options_("Valid options are", 120, 80)
  // {
  // 	using namespace boost::program_options;
  // 	// clang-format off
  // 	options_.add_options()
    // 		("denoise", value<std::string>(&denoise)->default_value("auto"),
  // 		 "")
  // 		("mode", value<std::string>(&mode_string),
  // 		 "Camera mode as W:H:bit-depth:packing, where packing is P (packed)
  // or U (unpacked)")
  // 		;
  // 	// clang-format on
  // }

  unsigned int width = 0;
  unsigned int height = 0;
  unsigned int framerate = 30;
  libcamera::Transform transform;
  float gain = 0;

  libcamera::controls::AeMeteringModeEnum metering =
      libcamera::controls::MeteringCentreWeighted;

  // Set the exposure mode (normal, sport)
  libcamera::controls::AeExposureModeEnum exposure =
      libcamera::controls::ExposureNormal;

  // Set the EV exposure compensation, where 0 = no change
  float ev = 0;

  // Set the AWB mode (auto, incandescent, tungsten, fluorescent, indoor,
  // daylight, cloudy, custom)
  libcamera::controls::AwbModeEnum awb = libcamera::controls::AwbAuto;

  // Set explict red and blue gains (disable the automatic AWB algorithm)
  float awb_gain_r = 0;

  // Set explict red and blue gains (disable the automatic AWB algorithm)
  float awb_gain_b = 0;

  // Adjust the brightness of the output images, in the range -1.0 to 1.0
  float brightness = 0;

  // Adjust the contrast of the output image, where 1.0 = normal contrast
  float contrast = 1.0;

  // Adjust the colour saturation of the output, where 1.0 = normal and 0.0 = greyscale
  float saturation = 1.0;

  // Adjust the sharpness of the output image, where 1.0 = normal sharpening
  float sharpness = 1.0;

  // Sets the Denoise operating mode: auto, off, cdn_off, cdn_fast, cdn_hq
  std::string denoise = "off";

  std::string mode_string;
  Mode mode;
};

enum DenoiseMode
{
  automatic,
  off,
  cdn_off,
  cdn_fast,
  cdn_hq
};