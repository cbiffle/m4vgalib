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
  .video_lead        = 19,
  .video_pixels      = 800,
  .hsync_polarity    = Timing::Polarity::positive,

  .vsync_start_line = 1,
  .vsync_end_line   = 1 + 4,
  .video_start_line = 1 + 4 + 23,
  .video_end_line   = 1 + 4 + 23 + 600,
  .vsync_polarity   = Timing::Polarity::positive,
};

Timing const timing_800x600_56hz = {
  .clock_config = {
    .crystal_hz = 8000000,  // external crystal Hz
    .crystal_divisor = 4,   // divide down to 2Mhz
    .vco_multiplier = 144,  // multiply up to 288MHz VCO
    .general_divisor = 2,   // divide by 2 for 144MHz CPU clock
    .pll48_divisor = 6,     // divide by 6 for 48MHz SDIO clock
    .ahb_divisor = 1,       // divide CPU clock by 1 for 144MHz AHB clock
    .apb1_divisor = 4,      // divide CPU clock by 4 for 36MHz APB1 clock.
    .apb2_divisor = 2,      // divide CPU clock by 2 for 72MHz APB2 clock.

    .flash_latency = 4,     // 4 wait states for 144MHz at 3.3V.
  },

  .cycles_per_pixel = 4,

  .line_pixels       = 1024,
  .sync_pixels       = 72,
  .back_porch_pixels = 128,
  .video_lead        = 22,
  .video_pixels      = 800,
  .hsync_polarity    = Timing::Polarity::positive,

  .vsync_start_line = 1,
  .vsync_end_line   = 1 + 2,
  .video_start_line = 1 + 2 + 22,
  .video_end_line   = 1 + 2 + 22 + 600,
  .vsync_polarity   = Timing::Polarity::positive,
};

}  // namespace vga
