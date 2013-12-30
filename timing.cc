#include "vga/timing.h"

namespace vga {

Timing const timing_vesa_640x480_60hz = {
  {
    8000000,  // external crystal Hz
    4,        // divide down to 2Mhz
    201,      // multiply up to 402MHz VCO
    4,        // divide by 4 for 100.5MHz CPU clock
    9,        // divide by 9 for 48MHz-ish SDIO clock
    1,        // divide CPU clock by 1 for 100.5MHz AHB clock
    4,        // divide CPU clock by 4 for 25.125MHz APB1 clock.
    2,        // divide CPU clock by 2 for 50.25MHz APB2 clock.

    3,        // 3 wait states for 100.5MHz at 3.3V.
  },

  800,   // line_pixels
  96,    // sync_pixels
  48,    // back_porch_pixels
  25,    // video_lead
  640,   // video_pixels,
  Timing::Polarity::negative,

  10,
  10 + 2,
  10 + 2 + 33,
  10 + 2 + 33 + 480,
  Timing::Polarity::negative,
};

Timing const timing_vesa_800x600_60hz = {
  {
    8000000,  // external crystal Hz
    4,        // divide down to 2Mhz
    160,      // multiply up to 320MHz VCO
    2,        // divide by 2 for 160MHz CPU clock
    7,        // divide by 7 for 48MHz-ish SDIO clock
    1,        // divide CPU clock by 1 for 160MHz AHB clock
    4,        // divide CPU clock by 4 for 40MHz APB1 clock.
    2,        // divide CPU clock by 2 for 80MHz APB2 clock.

    5,        // 5 wait states for 160MHz at 3.3V.
  },

  1056,  // line_pixels
  128,   // sync_pixels
  88,    // back_porch_pixels
  24,    // video_lead
  800,   // video_pixels,
  Timing::Polarity::positive,

  1,
  1 + 4,
  1 + 4 + 23,
  1 + 4 + 23 + 600,
  Timing::Polarity::positive,
};

}  // namespace vga
