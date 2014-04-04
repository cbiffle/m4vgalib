#include "vga/rast/text_10x16.h"

#include "vga/timing.h"
#include "vga/font_10x16.h"
#include "vga/rast/unpack_text_10p_attributed.h"

namespace vga {
namespace rast {

static constexpr unsigned glyph_cols = 10, glyph_rows = 16;
static constexpr unsigned chars_in_font = 256;

Text_10x16::Text_10x16(unsigned width, unsigned height, unsigned top_line)
  : _cols((width + (glyph_cols - 1)) / glyph_cols),
    _rows((height + (glyph_rows - 1)) / glyph_rows),
    _fb(nullptr),
    _font(nullptr),
    _top_line(top_line) {}

void Text_10x16::activate(Timing const &timing) {
  _font = new unsigned char[chars_in_font * glyph_rows];
  _fb = new unsigned[_cols * _rows];

  for (unsigned i = 0; i < chars_in_font * glyph_rows; ++i) {
    _font[i] = font_10x16[i];
  }
}

__attribute__((section(".ramcode")))
Rasterizer::LineShape Text_10x16::rasterize(unsigned line_number,
                                            Pixel *raster_target) {
  line_number -= _top_line;

  unsigned text_row = line_number / glyph_rows;
  unsigned row_in_glyph = line_number % glyph_rows;

  if (text_row >= _rows) return { 0, 0 };

  unsigned const *src = _fb + _cols * text_row;
  unsigned char const *font = _font + row_in_glyph * chars_in_font;

  unpack_text_10p_attributed_impl(src, font, raster_target, _cols);

  return { 0, _cols * glyph_cols };
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
  put_packed(col, row, (fore << 16) | (back << 8) | c);
}

void Text_10x16::put_packed(unsigned col, unsigned row,
                            unsigned p) {
  _fb[row * _cols + col] = p;
}

}  // namespace rast
}  // namespace vga
