#ifndef VGA_TIMING_H
#define VGA_TIMING_H

#include <cstdint>

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

  /*
   * The pixel clock is derived from the CPU clock by a fixed multiplier
   * (below), making the CPU clock configuration an integral part of the video
   * timing.
   */
  etl::stm32f4xx::ClockConfig clock_config;

  /*
   * Number of AHB cycles per pixel clock cycle.  Different scanout strategies
   * are invoked depending on this value.  Historically this was assumed to be
   * 4, but if you want a higher CPU : pixel clock ratio, go for it.
   *
   * Note that no current scanout strategy can achieve fewer than 4 AHB cycles
   * per pixel clock; attempt this and you'll hit an assert in configure_timing.
   */
  std::uint16_t cycles_per_pixel;

  /*
   * Horizontal timing, expressed in pixels.
   *
   * The horizontal sync pulse implicitly starts at pixel zero of the line.
   *
   * Some of this information is redundant; it's stored this way to avoid
   * having to rederive it in the driver.
   */
  std::uint16_t line_pixels;        // Total, including blanking.
  std::uint16_t sync_pixels;        // Length of pulse.
  std::uint16_t back_porch_pixels;  // Between end of sync and start of video.
  std::uint16_t video_lead;         // Fudge factor: nudge DMA back in time.
  std::uint16_t video_pixels;       // Maximum pixels in active video.
  Polarity      hsync_polarity;     // Polarity of hsync pulse.

  /*
   * Vertical timing, expressed in lines.
   *
   * Because vertical timing is done in software, it's a little more flexible
   * than horizontal timing.
   */
  std::uint16_t vsync_start_line;  // Top edge of sync pulse.
  std::uint16_t vsync_end_line;    // Bottom edge of sync pulse.
  std::uint16_t video_start_line;  // Top edge of active video.
  std::uint16_t video_end_line;    // Bottom edge of active video.
  Polarity      vsync_polarity;    // Polarity of vsync pulse.
};

/*
 * Canned timings for common modes at the assumed 8MHz crystal frequency.
 *
 * These use the highest CPU frequency at which we can reasonably approximate
 * the standard pixel clock (to within about 0.05%).
 */
extern Timing const timing_vesa_640x480_60hz;
extern Timing const timing_vesa_800x600_60hz;

// This non-VESA mode is useful if you need exactly 48 MHz for USB clock
extern Timing const timing_800x600_56hz;

}  // namespace vga

#endif  // VGA_TIMING_H
