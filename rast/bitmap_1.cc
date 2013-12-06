#include "vga/rast/bitmap_1.h"

#include "vga/vga.h"
#include "vga/unpack_1bpp.h"

namespace vga {
namespace rast {

Bitmap_1::Bitmap_1(unsigned width, unsigned height, unsigned top_line)
  : _lines(height),
    _bytes_per_line(width / 8),
    _top_line(top_line) {}


void Bitmap_1::activate(Timing const &) {
  _fb[0] = new unsigned char[_bytes_per_line * _lines];
  _fb[1] = new unsigned char[_bytes_per_line * _lines];
  _page1 = false;
  _clut[0] = 0;
  _clut[1] = 0xFF;

  for (unsigned i = 0; i < _bytes_per_line * _lines; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
}

void Bitmap_1::deactivate() {
  _fb[0] = _fb[1] = nullptr;
}

__attribute__((section(".ramcode")))
Rasterizer::LineShape Bitmap_1::rasterize(unsigned line_number, Pixel *target) {
  line_number -= _top_line;
  if (line_number >= _lines) return { 0, 0 };

  unsigned char const *src = _fb[_page1] + _bytes_per_line * line_number;

  unpack_1bpp_impl(src, _clut, target, _bytes_per_line);

  return { 0, _bytes_per_line * 8 };
}

Graphics1 Bitmap_1::make_bg_graphics() const {
  return Graphics1(_fb[!_page1], _bytes_per_line * 8, _lines,
                                 _bytes_per_line / 4);
}

void Bitmap_1::flip() {
  vga::sync_to_vblank();
  _page1 = !_page1;
}

void Bitmap_1::flip_now() {
  _page1 = !_page1;
}

void Bitmap_1::set_fg_color(Pixel c) {
  _clut[1] = c;
}

void Bitmap_1::set_bg_color(Pixel c) {
  _clut[0] = c;
}

}  // namespace rast
}  // namespace vga
