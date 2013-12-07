#include "vga/rast/direct.h"

#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/unpack_direct_x4.h"

namespace vga {
namespace rast {

Direct::Direct(unsigned width, unsigned height, unsigned top_line)
  : _width(width),
    _height(height),
    _top_line(top_line) {}


void Direct::activate(Timing const &) {
  _fb[0] = new unsigned char[_width * _height];
  _fb[1] = new unsigned char[_width * _height];
  _page1 = false;

  for (unsigned i = 0; i < _width * _height; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
}

void Direct::deactivate() {
  _fb[0] = _fb[1] = nullptr;
}

__attribute__((section(".ramcode")))
Rasterizer::LineShape Direct::rasterize(unsigned line_number, Pixel *target) {
  line_number -= _top_line;
  line_number /= 4;
  if (line_number >= _height) return { 0, 0 };

  unsigned char const *src = _fb[_page1] + _width * line_number;

  unpack_direct_x4_impl(src, target, _width);

  return { 0, _width * 4 };
}

void Direct::flip() {
  vga::sync_to_vblank();
  _page1 = !_page1;
}

void Direct::flip_now() {
  _page1 = !_page1;
}

}  // namespace rast
}  // namespace vga
