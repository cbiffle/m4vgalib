#ifndef VGA_TIMING_H
#define VGA_TIMING_H

#include "etl/stm32f4xx/rcc.h"

namespace vga {

/*
 * Describes the horizontal and vertical timing for a display mode, including
 * the outer bounds of active video.
 */
struct Timing {
  enum class Polarity {
    positive = 0,
    negative = 1,
  };

  typedef unsigned short ushort;  // a short-hand.  Get it?

  /*
   * The current scanout approach fixes the pixel clock at 4x the CPU (really
   * AHB) frequency.  The clock_config specifies how to achieve the desired
   * CPU clock, and thus implicitly defines the CPU clock.
   */
  etl::stm32f4xx::ClockConfig clock_config;

  /*
   * Horizontal timing.
   *
   * The horizontal sync pulse implicitly starts at pixel zero of the line.
   *
   * Some of this information is redundant; it's stored this way to avoid
   * having to rederive it in the driver.
   */
  ushort line_pixels;        // Total, including blanking.
  ushort sync_pixels;        // Length of pulse.
  ushort back_porch_pixels;  // Between end of sync and start of video.
  ushort video_lead;         // Fudge factor: nudge DMA start back in time.
  ushort video_pixels;       // Maximum pixels in active video.
  Polarity hsync_polarity;   // Polarity of hsync pulse.

  /*
   * Vertical timing
   *
   * Because vertical timing is done in software, it's a little more flexible
   * than horizontal timing.
   */
  ushort vsync_start_line;  // Top edge of sync pulse.
  ushort vsync_end_line;    // Bottom edge of sync pulse.
  ushort video_start_line;  // Top edge of active video.
  ushort video_end_line;    // Bottom edge of active video.
  Polarity vsync_polarity;  // Polarity of vsync pulse.
};

/*
 * Canned timings for common modes at our crystal frequency.
 */
extern Timing const timing_vesa_640x480_60hz;
extern Timing const timing_vesa_800x600_60hz;

}  // namespace vga

#endif  // VGA_TIMING_H
