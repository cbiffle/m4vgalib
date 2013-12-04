#include "vga/mode/raster_800x600x1.h"

#include "lib/stm32f4xx/rcc.h"
#include "vga/timing.h"
#include "vga/vga.h"

namespace vga {
namespace mode {

static constexpr unsigned cols = 800;
static constexpr unsigned rows = 600;

static stm32f4xx::ClockConfig const clock_cfg = {
  8000000,  // external crystal Hz
  8,        // divide down to 1Mhz
  320,      // multiply up to 320MHz VCO
  2,        // divide by 2 for 160MHz CPU clock
  7,        // divide by 7 for 48MHz-ish SDIO clock
  1,        // divide CPU clock by 1 for 160MHz AHB clock
  4,        // divide CPU clock by 4 for 40MHz APB1 clock.
  2,        // divide CPU clock by 2 for 80MHz APB2 clock.

  5,        // 5 wait states for 160MHz at 3.3V.
};

// We make this non-const to force it into RAM, because it's accessed from
// paths that won't enjoy the Flash wait states.
static Timing timing = {
  &clock_cfg,

  1056,  // line_pixels
  128,   // sync_pixels
  88,    // back_porch_pixels
  22,    // video_lead
  800,   // video_pixels,
  Timing::Polarity::positive,

  1,
  1 + 4,
  1 + 4 + 23,
  1 + 4 + 23 + 600,
  Timing::Polarity::positive,
};

Raster_800x600x1::Raster_800x600x1() : _rr(cols, rows) {}

void Raster_800x600x1::activate() {
  _rr.activate(timing);
}

__attribute__((section(".ramcode")))
void Raster_800x600x1::rasterize(unsigned line_number, Pixel *target) {
  // Adjust frame line to displayed line.
  line_number -= timing.video_start_line;
  (void) _rr.rasterize(line_number, target);
}

__attribute__((section(".ramcode")))
Timing const &Raster_800x600x1::get_timing() const {
  return timing;
}

Graphics1 Raster_800x600x1::make_bg_graphics() const {
  return _rr.make_bg_graphics();
}

void Raster_800x600x1::flip() {
  _rr.flip();
}

void Raster_800x600x1::set_fg_color(Pixel c) {
  _rr.set_fg_color(c);
}

void Raster_800x600x1::set_bg_color(Pixel c) {
  _rr.set_bg_color(c);
}

}  // namespace mode
}  // namespace vga
