#include "vga/timing.h"

namespace vga {

Timing const timing_vesa_640x480_60hz = {
  .clock_config = {
    .crystal_hz = 8000000,  // 8 MHz
    .crystal_divisor = 4,   // divide down to 2Mhz
    .vco_multiplier = 151,  // multiply up to 302MHz VCO
    .general_divisor = 2,   // divide VCO by 2 for 151MHz CPU clock
    .pll48_divisor = 7,     // divide VCO by 7 for 48MHz-ish SDIO clock
    
    .ahb_divisor = 1,       // run AHB at CPU rate.
    .apb1_divisor = 4,      // divide CPU clock by 4 for 37.75MHz APB1 clock.
    .apb2_divisor = 2,      // divide CPU clock by 2 for 75.5MHz APB2 clock.

    .flash_latency = 5,     // 5 wait states for 151MHz at 3.3V.
  },

  .cycles_per_pixel = 6,

  .line_pixels       = 800,
  .sync_pixels       = 96,
  .back_porch_pixels = 48,
  .video_lead        = 25,
  .video_pixels      = 640,
  .hsync_polarity    = Timing::Polarity::negative,

  .vsync_start_line = 10,
  .vsync_end_line   = 10 + 2,
  .video_start_line = 10 + 2 + 33,
  .video_end_line   = 10 + 2 + 33 + 480,
  .vsync_polarity   = Timing::Polarity::negative,
};

Timing const timing_vesa_800x600_60hz = {
  .clock_config = {
    .crystal_hz = 8000000,  // external crystal Hz
    .crystal_divisor = 4,   // divide down to 2Mhz
    .vco_multiplier = 160,  // multiply up to 320MHz VCO
    .general_divisor = 2,   // divide by 2 for 160MHz CPU clock
    .pll48_divisor = 7,     // divide by 7 for 48MHz-ish SDIO clock
    .ahb_divisor = 1,       // divide CPU clock by 1 for 160MHz AHB clock
    .apb1_divisor = 4,      // divide CPU clock by 4 for 40MHz APB1 clock.
    .apb2_divisor = 2,      // divide CPU clock by 2 for 80MHz APB2 clock.

    .flash_latency = 5,     // 5 wait states for 160MHz at 3.3V.
  },

  .cycles_per_pixel = 4,

  .line_pixels       = 1056,
  .sync_pixels       = 128,
  .back_porch_pixels = 88,
  .video_lead        = 18,
  .video_pixels      = 800,
  .hsync_polarity    = Timing::Polarity::positive,

  .vsync_start_line = 1,
  .vsync_end_line   = 1 + 4,
  .video_start_line = 1 + 4 + 23,
  .video_end_line   = 1 + 4 + 23 + 600,
  .vsync_polarity   = Timing::Polarity::positive,
};

}  // namespace vga
