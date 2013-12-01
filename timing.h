#ifndef VGA_TIMING_H
#define VGA_TIMING_H

#include "lib/stm32f4xx/rcc.h"

namespace vga {

struct Timing {
  enum class Polarity {
    positive = 0,
    negative = 1,
  };

  typedef unsigned short ushort;

  stm32f4xx::ClockConfig const *clock_config;

  ushort line_pixels;
  ushort sync_pixels;
  ushort back_porch_pixels;
  ushort video_lead;
  ushort video_pixels;
  Polarity hsync_polarity;

  ushort vsync_start_line;
  ushort vsync_end_line;
  ushort video_start_line;
  ushort video_end_line;
  Polarity vsync_polarity;
};

}  // namespace vga

#endif  // VGA_TIMING_H
