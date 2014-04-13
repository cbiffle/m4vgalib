#include "vga/mode/raster_640x480x1.h"

#include "etl/stm32f4xx/rcc.h"
#include "vga/timing.h"

namespace vga {
namespace mode {

static constexpr unsigned cols = 640;
static constexpr unsigned rows = 480;

static etl::stm32f4xx::ClockConfig const clock_cfg = {
  8000000,  // external crystal Hz
  8,        // divide down to 1Mhz
  201,      // multiply up to 201MHz VCO
  2,        // divide by 2 for 100.5MHz CPU clock
  3,        // divide by 3 for 48MHz-ish SDIO clock
  1,        // divide CPU clock by 1 for 100.5MHz AHB clock
  4,        // divide CPU clock by 4 for 25.125MHz APB1 clock.
  2,        // divide CPU clock by 2 for 50.25MHz APB2 clock.

  3,        // 3 wait states for 100.5MHz at 3.3V.
};

// We make this non-const to force it into RAM, because it's accessed from
// paths that won't enjoy the Flash wait states.
static Timing timing = {
  &clock_cfg,

  800,   // line_pixels
  96,    // sync_pixels
  48,    // back_porch_pixels
  22,    // video_lead
  640,   // video_pixels,
  Timing::Polarity::negative,

  10,
  10 + 2,
  10 + 2 + 33,
  10 + 2 + 33 + 480,
  Timing::Polarity::negative,
};

Raster_640x480x1::Raster_640x480x1()
  : Raster_1(cols, rows, timing) {}

}  // namespace mode
}  // namespace vga
