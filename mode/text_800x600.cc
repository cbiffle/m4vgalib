#include "vga/mode/text_800x600.h"

#include "lib/stm32f4xx/rcc.h"
#include "vga/timing.h"
#include "vga/unpack_text_10p_attributed.h"

#include "vga/font_10x16.h"
#include "vga/vga.h"

namespace vga {
namespace mode {

static constexpr unsigned cols = 800 / 10;
static constexpr unsigned rows = (600 + 15) / 16;

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

void Text_800x600::activate() {
  _font = new unsigned char[4096];
  _framebuffer = new unsigned[cols * rows];

  // Copying the font into RAM allows for mutable fonts down the road,
  // and also saves 96 cycles per line.  (Actually a surprisingly low number.)
  for (unsigned i = 0; i < 4096; ++i) {
    _font[i] = font_10x16[i];
  }
}

void Text_800x600::deactivate() {
  _framebuffer = nullptr;
  _font = nullptr;
}

__attribute__((section(".ramcode")))
void Text_800x600::rasterize(unsigned line_number, Pixel *target) {
  line_number -= timing.video_start_line;

  unsigned text_row = line_number / 16;
  unsigned row_in_glyph = line_number % 16;

  unsigned const *src = _framebuffer + cols * text_row;
  unsigned char const *font = _font + row_in_glyph * 256;

  unpack_text_10p_attributed_impl(src, font, target, cols);
}

__attribute__((section(".ramcode")))
Timing const &Text_800x600::get_timing() const {
  return timing;
}

void Text_800x600::clear_framebuffer(Pixel bg) {
  unsigned word = bg << 8 | ' ';
  for (unsigned i = 0; i < cols * rows; ++i) {
    _framebuffer[i] = word;
  }
}

void Text_800x600::put_char(unsigned col, unsigned row,
                            Pixel fore, Pixel back, char c) {
  wait_for_vblank();
  _framebuffer[row * cols + col] = (fore << 16) | (back << 8) | c;
}

}  // namespace mode
}  // namespace vga
