#include "vga/rast/solid_color.h"

#include "vga/vga.h"

namespace vga {
namespace rast {

SolidColor::SolidColor(unsigned width, Pixel color)
  : _width(width),
    _color(color) {}

__attribute__((section(".ramcode")))
Rasterizer::RasterInfo SolidColor::rasterize(unsigned, Pixel *target) {
  target[0] = _color;

  return {
    .offset = 0,
    .length = 1,
    .stretch_cycles = (_width - 1) * 4,
    .repeat_lines = 1000,
  };
}

}  // namespace rast
}  // namespace vga
