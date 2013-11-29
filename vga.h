#ifndef VGA_VGA_H
#define VGA_VGA_H

#include "lib/stm32f4xx/rcc.h"

namespace vga {

void init();

void video_on();
void video_off();

struct VideoMode {
  enum class Polarity {
    positive = 0,
    negative = 1,
  };

  typedef unsigned short ushort;

  stm32f4xx::ClockConfig const &clock_config;

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

void select_mode(VideoMode const &);

}  // namespace vga

#endif  // VGA_VGA_H
