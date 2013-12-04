#include "vga/rast/text_10x16.h"

#include "vga/timing.h"
#include "vga/font_10x16.h"
#include "vga/unpack_text_10p_attributed.h"

namespace vga {
namespace rast {

Text_10x16::Text_10x16()
  : _cols(0),
    _rows(0),
    _fb(nullptr),
    _font(nullptr) {}

void Text_10x16::activate(Timing const &timing) {
  unsigned screen_rows =
      timing.video_end_line - timing.video_start_line;

  unsigned cols = timing.video_pixels / 10;
  unsigned rows = screen_rows / 16;

  _font = new unsigned char[256 * 16];
  _fb = new unsigned[cols * rows];
  _cols = cols;
  _rows = rows;

  for (unsigned i = 0; i < 256 * 16; ++i) {
    _font[i] = font_10x16[i];
  }
}

__attribute__((section(".ramcode")))
Rasterizer::LineShape Text_10x16::rasterize(unsigned line_number,
                                            Pixel *raster_target) {
  unsigned text_row = line_number / 16;
  unsigned row_in_glyph = line_number % 16;

  unsigned const *src = _fb + _cols * text_row;
  unsigned char const *font = _font + row_in_glyph * 256;

  unpack_text_10p_attributed_impl(src, font, raster_target, _cols);

  return { 0, _cols * 10 };
}

void Text_10x16::deactivate() {
  _font = nullptr;
  _fb = nullptr;
  _cols = 0;
}

void Text_10x16::clear_framebuffer(Pixel bg) {
  unsigned word = bg << 8 | ' ';
  for (unsigned i = 0; i < _cols * _rows; ++i) {
    _fb[i] = word;
  }
}

void Text_10x16::put_char(unsigned col, unsigned row,
                          Pixel fore, Pixel back,
                          char c) {
  _fb[row * _cols + col] = (fore << 16) | (back << 8) | c;
}

}  // namespace rast
}  // namespace vga
