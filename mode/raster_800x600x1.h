#ifndef VGA_MODE_RASTER_800X600X1_H
#define VGA_MODE_RASTER_800X600X1_H

#include "vga/mode.h"
#include "vga/graphics_1.h"

namespace vga {
namespace mode {

class Raster_800x600x1 : public Mode {
public:
  virtual void activate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

  Graphics1 make_graphics() const;

private:
  unsigned char *_framebuffer;
  Pixel _clut[2];
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_RASTER_800X600X1_H
