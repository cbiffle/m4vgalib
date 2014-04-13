#include "vga/rast/solid_color.h"

#include "vga/vga.h"

namespace vga {
namespace rast {

SolidColor::SolidColor(unsigned width, Pixel color)
  : _width(width),
    _color(color) {}


void SolidColor::activate(Timing const &) {
}

void SolidColor::deactivate() {
}

__attribute__((section(".ramcode")))
Rasterizer::LineShape SolidColor::rasterize(unsigned, Pixel *target) {
  unsigned words = _width / 4;
  unsigned *target32 =
      reinterpret_cast<unsigned *>(static_cast<void *>(target));

  unsigned color32 = _color | (_color << 8) | (_color << 16) | (_color << 24);
  for (unsigned i = 0; i < words; ++i) {
    target32[i] = color32;
  }

  return { 0, words * 4 };
}

void SolidColor::set_color(Pixel c) {
  _color = c;
}

}  // namespace rast
}  // namespace vga
