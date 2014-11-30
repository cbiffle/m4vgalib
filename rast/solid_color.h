#ifndef VGA_RAST_SOLID_COLOR_H
#define VGA_RAST_SOLID_COLOR_H

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

class SolidColor : public Rasterizer {
public:
  SolidColor(unsigned width, Pixel color);

  virtual void activate(Timing const &) override;
  virtual LineShape rasterize(unsigned, Pixel *) override;
  virtual void deactivate() override;

  void set_color(Pixel);

private:
  unsigned _width;
  Pixel _color;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_SOLID_COLOR_H
