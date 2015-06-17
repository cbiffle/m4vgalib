#include "vga/rast/solid_color.h"

#include "vga/vga.h"

namespace vga {
namespace rast {

SolidColor::SolidColor(unsigned width, Pixel color)
  : _width(width),
    _color(color) {}

__attribute__((section(".ramcode")))
Rasterizer::RasterInfo SolidColor::rasterize(unsigned cycles_per_pixel,
                                             unsigned,
                                             Pixel *target) {
  target[0] = _color;

  return {
    .offset = 0,
    .length = 1,
    .cycles_per_pixel = cycles_per_pixel * _width,
    .repeat_lines = 1000,
  };
}

}  // namespace rast
}  // namespace vga
