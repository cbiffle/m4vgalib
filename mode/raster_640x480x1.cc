#include "vga/mode/raster_640x480x1.h"

#include "lib/stm32f4xx/rcc.h"
#include "vga/timing.h"
#include "vga/unpack_1bpp.h"
#include "vga/vga.h"

namespace vga {
namespace mode {

static constexpr unsigned cols = 640;
static constexpr unsigned rows = 480;
static constexpr unsigned fb_stride = cols / 8;

static stm32f4xx::ClockConfig const clock_cfg = {
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

void Raster_640x480x1::activate() {
  _framebuffer[0] = new Pixel[fb_stride * rows];
  _framebuffer[1] = new Pixel[fb_stride * rows];
  _page1 = false;
  _clut[0] = 0;
  _clut[1] = 0xFF;

  for (unsigned i = 0; i < fb_stride * rows; ++i) {
    _framebuffer[0][i] = 0;
    _framebuffer[1][i] = 0;
  }
}

__attribute__((section(".ramcode")))
void Raster_640x480x1::rasterize(unsigned line_number, Pixel *target) {
  // Adjust frame line to displayed line.
  line_number -= timing.video_start_line;
  unsigned char const *src = _framebuffer[_page1] + fb_stride * line_number;

  unpack_1bpp_impl(src, _clut, target, fb_stride);
}

__attribute__((section(".ramcode")))
Timing const &Raster_640x480x1::get_timing() const {
  return timing;
}

Graphics1 Raster_640x480x1::make_bg_graphics() const {
  return Graphics1(_framebuffer[!_page1], 640, 480, 640 / 32);
}

void Raster_640x480x1::flip() {
  vga::wait_for_vblank();
  _page1 = !_page1;
}

void Raster_640x480x1::set_fg_color(Pixel c) {
  _clut[1] = c;
}

void Raster_640x480x1::set_bg_color(Pixel c) {
  _clut[0] = c;
}

}  // namespace mode
}  // namespace vga
